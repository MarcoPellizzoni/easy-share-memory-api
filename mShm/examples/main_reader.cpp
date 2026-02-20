#include <iostream>
#include <string>
#include "mshm.h"
#include <stddef.h>


struct t_data_shm
{
    int toggle = 0;
    int cycle = 0;
};

int main()
{
    std::string key;
    t_data_shm p_shm;


    // NOTE DI UTILIZZO CON CONTAINER:
    // 1. Fare bind mount della shmem precedentemente aperta (il file deve esistere)
    // 2. Dare i permessi SE Linux al container per quel particolare file
    // Operativamente:
    //          -v /dev/shm/nome_shmem_nell_host:/dev/shm/nome_shmem_nel_container:Z
    //          "nome_shmem_nel_container" sarà il nome con cui sarà visibile la shm "nome_shmem_nell_host"
    //          La ":Z" finale aggiunge i permessi per le operazione sulla shm


    /////////////////////////////////////////
    // FUNCTION VERSION
    std::string shm_name = "test";
    mshm::mshm_handle test_shm = nullptr;
    mshm::Return ret = mshm::shmem_open(test_shm, shm_name.c_str(), sizeof(t_data_shm));

    if(ret.error_code != mshm::SHMEM_OK)
    {
        std::cout << ret.error_string << std::endl;
        std::getline(std::cin, key);
    }

    while (true)
    {
        ret = mshm::shmem_read(test_shm, &p_shm.cycle, sizeof(p_shm.cycle), offsetof(t_data_shm, cycle));

        std::cout << p_shm.toggle << " " << p_shm.cycle << std::endl;

        if (key == "q")
        {
            break;
        }
    }

    ret = mshm::shmem_close(test_shm);


    //if(ret.error_code != mshm::SHMEM_OK)
    //{
    //    std::cout << ret.error_string << std::endl;
    //    std::getline(std::cin, key);
    //}

    ///////////////////////////////////////////
    //// CLASS VERSION
    //mshm::SharedMemory<t_data_shm> data_shm{ "/test_class_shm" };
    //auto retu = data_shm.open();

    //while (true)
    //{
    //    std::getline(std::cin, key);

    //    ret = data_shm.read(p_shm);
    //    std::cout << p_shm.toggle << " " << p_shm.cycle << std::endl;
    //
    //    p_shm.toggle = !p_shm.toggle;
    //    p_shm.cycle++;

    //    ret = data_shm.write(p_shm);

    //    if (key == "q")
    //    {
    //        break;
    //    }
    //}


}
