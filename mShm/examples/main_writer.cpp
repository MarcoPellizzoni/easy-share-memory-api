#include <iostream>
#include <string>
#include "mshm.h"

struct t_data_shm
{
    int toggle = 0;
    int cycle = 0;
};

int main()
{
    std::string key;

    std::string shm_name = "test";
    t_data_shm p_shm;

    /////////////////////////////////////////
    // FUNCTION VERSION
    mshm::mshm_handle test_shm = nullptr;

    mshm::Return ret = mshm::shmem_open(test_shm, shm_name.c_str(), sizeof(t_data_shm));

    if(ret.error_code != mshm::SHMEM_OK)
    {
        std::cout << ret.error_string << std::endl;
        std::getline(std::cin, key);
    }

    while (true)
    {
        p_shm.toggle = !p_shm.toggle;
        p_shm.cycle++;
        
        ret = mshm::shmem_write(test_shm, &p_shm.cycle, sizeof(p_shm.cycle), offsetof(t_data_shm, cycle));

        if (key == "q")
        {
            break;
        }
    }


    ret = mshm::shmem_close(test_shm);

    ret = mshm::shmem_delete(shm_name.c_str());

    /////////////////////////////////////////
    // CLASS VERSION
    //mshm::SharedMemory<t_data_shm> data_shm{shm_name.c_str()};
    //ret = data_shm.open();
//
    //while (true)
    //{
    //    std::getline(std::cin, key);
//
    //    ret = data_shm.read(p_shm);
    //    std::cout << p_shm.toggle << " " << p_shm.cycle << std::endl;
//
    //    p_shm.toggle = !p_shm.toggle;
    //    p_shm.cycle++;
//
    //    ret = data_shm.write(p_shm);
//
    //    if (key == "q")
    //    {
    //        break;
    //    }
    //}


}
