#include "mshm.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <cctype>


using namespace mshm;

struct shmem_internal_t
{
    pthread_mutex_t mutex;     // mutex salvato nella memoria condivisa ad esso associato
    size_t data_size;          // dimensione massima dati
    unsigned char data[];      // buffer variabile
};

struct t_shmem_handle
{
    shmem_internal_t* shm = nullptr;
    int h_fd = -1;
    size_t total_size = 0;
};

Return check_handle(mshm_handle mshm)
{
    Return ret;
    ret.error_code = SHMEM_OK;
    ret.error_string = "No Error";

    if (mshm == nullptr)
    {
        ret.error_code = SHMEM_ERR_NOT_OPEN;
        ret.error_string = "Handel is NULL";
        return ret;
    }

    t_shmem_handle* handle = (t_shmem_handle*)(mshm);

    if (handle->shm == nullptr)
    {
        ret.error_code = SHMEM_ERR_NOT_OPEN;
        ret.error_string = "Shmem pointer is null";
        return ret;
    }

    if (handle->h_fd == -1)
    {
        ret.error_code = SHMEM_ERR_NOT_OPEN;
        ret.error_string = "File descriptor is not valid";
        return ret;
    }

    return ret;
}


bool is_valid_shm_name(const std::string& name, std::string* error = nullptr)
{
    // Must not be empty
    if (name.empty())
    {
        if (error) *error = "Shared memory name cannot be empty";
        return false;
    }

    // Validate characters (POSIX suggests limited portable charset)
    for (char c : name)
    {
        if (!std::isalnum((unsigned char)c) && c!='_' && c!='-')
        {
            if (error) *error = "Shared memory name contains invalid character: '" + std::string(1, c) + "'";
            return false;
        }
    }

    // Length check (optional but good practice)
    if (name.length() > 255)
    {
        if (error) *error = "Shared memory name too long (max ~255)";
        return false;
    }

    return true;
}



Return mshm::shmem_open(mshm_handle& mshm, const char* name, size_t user_data_size)
{
    Return ret;

    std::string name_error_description;

    if(!is_valid_shm_name(name, &name_error_description))
    {
        mshm = nullptr;
        ret.error_code = SHMEM_ERR_PARAM;
        ret.error_string = name_error_description;
        return ret;
    }

    if (user_data_size <= 0)
    {
        mshm = nullptr;
        ret.error_code = SHMEM_ERR_PARAM;
        ret.error_string = "Size is not grater then 0";
        return ret;
    }

    t_shmem_handle* handle = new t_shmem_handle();
    handle->total_size = sizeof(shmem_internal_t) + user_data_size;

    int init = 1;

    // add / prefix (needed for posix) -> the file will be placed in /dev/shm
    char local_name[512];
    snprintf(local_name, sizeof(local_name), "/%s", name);

    // create file descriptor for shared memory
    handle->h_fd = shm_open(local_name, O_CREAT | O_RDWR | O_EXCL, 0660); // -1 = failure

    if (handle->h_fd < 0 && errno == EEXIST) // file already exists
    {
        // open file descriptor
        handle->h_fd = shm_open(local_name, O_CREAT | O_RDWR, 0660); // -1 = failure
        init = 0;
    }

    if (handle->h_fd < 0)
    {
        mshm = nullptr;
        ret.error_code = SHMEM_ERR_OPEN;
        ret.error_string = strerror(errno);
        delete handle;
        return ret;
    }

    // Setup dimention dimention
    if (ftruncate(handle->h_fd, handle->total_size) != 0) // 0 = success
    {
        close(handle->h_fd);
        handle->h_fd = -1;
        mshm = nullptr;
        ret.error_code = SHMEM_ERR_FTRUNC;
        ret.error_string = strerror(errno);
        delete handle;
        return ret;
    }

    // Memory mapping
    handle->shm = (shmem_internal_t*)mmap( // MAP_FAILED = failure
        NULL,
        handle->total_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        handle->h_fd,
        0
    );

    if (handle->shm == MAP_FAILED)
    {
        mshm = nullptr;
        handle->shm = NULL;
        close(handle->h_fd);
        handle->h_fd = -1;
        ret.error_code = SHMEM_ERR_MMAP;
        ret.error_string = strerror(errno);
        delete handle;
        return ret;
    }

    if (init) // if new shm, create the interprocess mutex
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&handle->shm->mutex, &attr);
        pthread_mutexattr_destroy(&attr);

        handle->shm->data_size = user_data_size;
        memset(handle->shm->data, 0, user_data_size);
    }

    *mshm = handle;
    ret.error_code = SHMEM_OK;
    ret.error_string = "No Error";

    return ret;
}


Return mshm::shmem_close(mshm_handle mshm)
{
    Return ret = check_handle(mshm);

    if (ret.error_code != SHMEM_OK)
    {
        return ret;
    }

    t_shmem_handle* handle = (t_shmem_handle*)(mshm);

    int error = munmap(handle->shm, handle->total_size); // 0 = success

    if (error)
    {
        ret.error_code = SHMEM_ERR_NOT_OPEN;
        ret.error_string = strerror(errno);
        return ret;
    }

    error = close(handle->h_fd); // 0 = success

    if (error)
    {
        ret.error_code = SHMEM_ERR_NOT_OPEN;
        ret.error_string = strerror(errno);
        return ret;
    }

    handle->shm = NULL;
    handle->h_fd = -1;
    handle->total_size = 0;

    return ret;
}

Return mshm::shmem_delete(const char* name)
{
    Return ret;

    if (name == nullptr)
    {
        ret.error_code = SHMEM_ERR_PARAM;
        ret.error_string = "Invalid parameters";
        return ret;
    }

    int error = shm_unlink(name); // 0 = success

    if (error)
    {
        ret.error_code = SHMEM_ERR_DELETE;
        ret.error_string = strerror(errno);
        return ret;
    }

    return ret;
}


Return mshm::shmem_write(mshm_handle mshm, const void* src, size_t size, uint64_t offset)
{
    Return ret = check_handle(mshm);

    if (ret.error_code != SHMEM_OK)
    {
        return ret;
    }

    if (src == nullptr)
    {
        ret.error_code = SHMEM_ERR_PARAM;
        ret.error_string = "src in NULL";
        return ret;
    }

    t_shmem_handle* handle = (t_shmem_handle*)(mshm);

    if (offset + size > handle->shm->data_size)
    {
        ret.error_code = SHMEM_ERR_PARAM;
        ret.error_string = "Size plus offset is exceeding the shmem size";
        return ret;
    }

    int error = pthread_mutex_lock(&handle->shm->mutex); // 0 = success

    if (error)
    {
        (error == EDEADLK) ? ret.error_string = "Mutex is dead lock" : ret.error_string = "Mutex not properly initialized";
        ret.error_code = SHMEM_ERR_MUTEX;
        return ret;
    }

    memcpy(&handle->shm->data[offset], src, size);

    error = pthread_mutex_unlock(&handle->shm->mutex); // 0 = success

    if (error)
    {
        error == EPERM ? ret.error_string = "The calling thread does not own the mutex" : ret.error_string = "Mutex not properly initialized";
        ret.error_code = SHMEM_ERR_MUTEX;
        return ret;
    }

    return ret;
}


Return mshm::shmem_read(mshm_handle mshm, void* dst, size_t size, uint64_t offset)
{
    Return ret = check_handle(mshm);

    if (ret.error_code != SHMEM_OK)
    {
        return ret;
    }

    if (dst == nullptr)
    {
        ret.error_code = SHMEM_ERR_PARAM;
        ret.error_string = "dst in NULL";
        return ret;
    }

    t_shmem_handle* handle = (t_shmem_handle*)(mshm);

    if (offset + size > handle->shm->data_size)
    {
        ret.error_code = SHMEM_ERR_PARAM;
        ret.error_string = "Size plus offset is exceeding the shmem size";
        return ret;
    }

    int error = pthread_mutex_lock(&handle->shm->mutex); // 0 = success

    if (error)
    {
        (error == EDEADLK) ? ret.error_string = "Mutex is dead lock" : ret.error_string = "Mutex not properly initialized";
        ret.error_code = SHMEM_ERR_MUTEX;
        return ret;
    }

    memcpy(dst, &handle->shm->data[offset], size);

    error = pthread_mutex_unlock(&handle->shm->mutex); // 0 = success

    if (error)
    {
        error == EPERM ? ret.error_string = "The calling thread does not own the mutex" : ret.error_string = "Mutex not properly initialized";
        ret.error_code = SHMEM_ERR_MUTEX;
        return ret;
    }

    return ret;
}

