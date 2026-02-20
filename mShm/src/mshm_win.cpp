#define _CRT_SECURE_NO_WARNINGS

#include "mshm.h"

#include <windows.h>
#include <strsafe.h>
#include <string.h>

using namespace mshm;

struct t_shmem_internal
{
    size_t data_size;
    unsigned char data[];
};

struct t_shmem_handle
{
    t_shmem_internal* shm = nullptr;
    HANDLE h_map = nullptr;
    HANDLE h_mutex = nullptr;
    size_t total_size = 0;
};


std::string get_last_error_message()
{
    DWORD dw = GetLastError();
  
    LPVOID lpMsgBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);

    std::string msg = (char*)lpMsgBuf;
    
    LocalFree(lpMsgBuf);

    return msg;
}

Return validate_mshm_handle(mshm_handle mshm)
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

    if (handle->h_map == NULL)
    {
        ret.error_code = SHMEM_ERR_NOT_OPEN;
        ret.error_string = "File mapping handle is NULL";
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
    handle->total_size = sizeof(t_shmem_internal) + user_data_size;

    // aggiungo prefisso di visibilità al nome: 
    // Local\ -> visibilità solo nella sessione corrente
    // Global\ -> visibilità tra sessioni diverse 
    char local_name[512];
    snprintf(local_name, sizeof(local_name), "Local\\%s", name);
    
    // Crea o apre file mapping (apre se "name" esiste già)
    handle->h_map = CreateFileMappingA(
        INVALID_HANDLE_VALUE, 
        NULL,
        PAGE_READWRITE, // TODO aggiungere switch modalità di apertura come parametro della funzione 
        (DWORD)((handle->total_size >> 32) & 0xFFFFFFFF),
        (DWORD)(handle->total_size & 0xFFFFFFFF),
        local_name
    );

    // se il file esisteva già allora non va inizializzato
    int init = ( GetLastError() == ERROR_ALREADY_EXISTS ) ? 0 : 1;

    if (handle->h_map == NULL)
    {
        mshm = nullptr;
        ret.error_code = SHMEM_ERR_OPEN;
        ret.error_string = get_last_error_message();
        delete handle;
        return ret;
    }

    // Mappatura memoria e file
    handle->shm = (t_shmem_internal*)MapViewOfFile(
        handle->h_map,
        FILE_MAP_ALL_ACCESS, // todo: switch modalità read o write
        0,
        0,
        handle->total_size
    );

    if (handle->shm == NULL) 
    {
        mshm = nullptr;
        ret.error_code = SHMEM_ERR_MMAP;
        ret.error_string = get_last_error_message();
        CloseHandle(handle->h_map);
        delete handle;
        return ret;
    }

    // Crea o apre il mutex (nome derivato dal nome della shared memory)
    char mutex_name[512];
    snprintf(mutex_name, sizeof(mutex_name), "%s_mutex", name);

    if (init)
    {
        handle->h_mutex = CreateMutexA(
            NULL,
            FALSE,
            mutex_name
        );
    }
    else
    {
        handle->h_mutex = OpenMutexA(
            SYNCHRONIZE,
            FALSE,
            mutex_name
        );
    }

    if (handle->h_mutex == NULL)
    {
        mshm = nullptr;
        ret.error_code = SHMEM_ERR_MUTEX;
        ret.error_string = get_last_error_message();
        UnmapViewOfFile(handle->shm);
        CloseHandle(handle->h_map);
        delete handle;
        return ret;
    }

    if (init) 
    {
        handle->shm->data_size = user_data_size;
        memset(handle->shm->data, 0, user_data_size);
    }

    mshm = handle;
    ret.error_code = SHMEM_OK;
    ret.error_string = "No Error";

    return ret;
}


Return mshm::shmem_close(mshm_handle mshm)
{
    Return ret = validate_mshm_handle(mshm);

    if (ret.error_code != SHMEM_OK)
    {
        return ret;
    }

    t_shmem_handle* handle = (t_shmem_handle*)(mshm);

    BOOL success = UnmapViewOfFile(handle->shm);

    if (!success)
    {
        ret.error_code = SHMEM_ERR_NOT_OPEN;
        ret.error_string = get_last_error_message();
        return ret;
    }

    success = CloseHandle(handle->h_map);
    
    if (!success)
    {
        ret.error_code = SHMEM_ERR_NOT_OPEN;
        ret.error_string = get_last_error_message();
        return ret;
    }

    success = CloseHandle(handle->h_mutex);

    if (!success)
    {
        ret.error_code = SHMEM_ERR_NOT_OPEN;
        ret.error_string = get_last_error_message();
        return ret;
    }

    handle->h_mutex = NULL;
    handle->shm = NULL;
    handle->h_map = NULL;
    handle->total_size = 0;

    return ret;
}


Return mshm::shmem_write(mshm_handle mshm, const void* src, size_t size, uint64_t offset)
{
    Return ret = validate_mshm_handle(mshm);

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
        ret.error_string = "Size plus offset is exceed the shmem size";
        return ret;
    }

    DWORD wait = WaitForSingleObject(handle->h_mutex, INFINITE);

    // if(wait == WAIT_TIMEOUT) --> INFINITE timeout. This case should not happen ever

    if (wait == WAIT_ABANDONED)
    {
        ret.error_code = SHMEM_ERR_MUTEX;
        ret.error_string = "Mutex was not released by the thread that owned the mutex before the owning thread terminated";
        return ret;
    }

    if (wait == WAIT_FAILED)
    {
        ret.error_code = SHMEM_ERR_MUTEX;
        ret.error_string = get_last_error_message();
        return ret;
    }

    memcpy(&handle->shm->data[offset], src, size);

    BOOL success = ReleaseMutex(handle->h_mutex);

    if (!success)
    {
        ret.error_code = SHMEM_ERR_MUTEX;
        ret.error_string = get_last_error_message();
        return ret;
    }

    return ret;
}

Return mshm::shmem_read(mshm_handle mshm, void* dst, size_t size, uint64_t offset)
{
    Return ret = validate_mshm_handle(mshm);

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
        ret.error_string = "Size plus offset exceed the shmem size";
        return ret;
    }

    DWORD wait = WaitForSingleObject(handle->h_mutex, INFINITE);

    // if(wait == WAIT_TIMEOUT) --> INFINITE timeout. This case should not happen ever

    if (wait == WAIT_ABANDONED)
    {
        ret.error_code = SHMEM_ERR_MUTEX;
        ret.error_string = "Mutex was not released by the thread that owned the mutex before the owning thread terminated";
        return ret;
    }

    if (wait == WAIT_FAILED)
    {
        ret.error_code = SHMEM_ERR_MUTEX;
        ret.error_string = get_last_error_message();
        return ret;
    }

    memcpy(dst, &handle->shm->data[offset], size);

    BOOL success = ReleaseMutex(handle->h_mutex);

    if (!success)
    {
        ret.error_code = SHMEM_ERR_MUTEX;
        ret.error_string = get_last_error_message();
        return ret;
    }

    return ret;
}

Return mshm::shmem_delete(const char* name)
{
    Return ret;
    ret.error_code = SHMEM_ERR_DELETE;
    ret.error_string = "On windows shmem delete is useless. The memory is automatically deleted upon closing.";

    return ret;
}
