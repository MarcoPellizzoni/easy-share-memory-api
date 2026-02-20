/**
    @file      mshm.h
    @brief     Tool to open shared memory using new OS api
    @details   ~ 
    @author    Marco Pellizzoni
**/

#ifndef SHMEM_H
#define SHMEM_H

#if defined(WIN32) && defined(_BUILD_MSHM_LIB_DLL)
	#if defined(_MSHM_LIB_EXPORT)
		#define MSHMAPI __declspec(dllexport)
	#else
		#define MSHMAPI __declspec(dllimport)
	#endif
#else
	#define MSHMAPI
#endif


#include <string>
#include <cstdint>

namespace mshm
{
    enum ErrorCode
    {
        SHMEM_OK,
        SHMEM_ERR_PARAM,
        SHMEM_ERR_OPEN,
        SHMEM_ERR_FTRUNC,
        SHMEM_ERR_MMAP,
        SHMEM_ERR_MUTEX,
        SHMEM_ERR_NOT_OPEN,
        SHMEM_ERR_SIZE,
        SHMEM_ERR_COPY,
        SHMEM_ERR_DELETE
    };

    struct Return
    {
        ErrorCode   error_code;
        std::string error_string;
    };

    typedef void* mshm_handle;

    MSHMAPI Return shmem_open(mshm_handle& handle, const char* name, size_t size);

    MSHMAPI Return shmem_close(mshm_handle handle);

    MSHMAPI Return shmem_write(mshm_handle shm, const void* src, size_t size, uint64_t offset = 0);

    MSHMAPI Return shmem_read(mshm_handle shm, void* dst, size_t size, uint64_t offset = 0);

    MSHMAPI Return shmem_delete(const char* name);

    //template <typename T>
    //class SharedMemory
    //{
    //public:
    //    explicit SharedMemory(const std::string& name)
    //        : _name(name), _mshm(nullptr)
    //    {
    //    }

    //    ~SharedMemory()
    //    {
    //        close();
    //        shmem_delete(_name.c_str());
    //    }

    //    Return open()
    //    {
    //        return _status = shmem_open(&_mshm, _name.c_str(), sizeof(T));
    //    }

    //    Return close()
    //    {
    //        return _status = shmem_close(_mshm);
    //    }

    //    Return write(const T& src)
    //    {
    //        return _status = shmem_write(_mshm, &src, sizeof(T));
    //    }

    //    Return read(T& dst)
    //    {
    //        return _status = shmem_read(_mshm, &dst, sizeof(T));
    //    }

    //    Return status()
    //    {
    //        return _status;
    //    }

    //    std::string name()
    //    {
    //        return _name;
    //    }

    //private:
    //    Return      _status;
    //    std::string _name;
    //    mshm_handle _mshm;
    //};
}

#endif
