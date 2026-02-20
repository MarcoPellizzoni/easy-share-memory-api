
Need to use shared memory in a C++ project without reinventing the wheel? 

This API wraps all the system calls into friendly C++ function calls. Runs both in windows and linux.

You are only required to:
  1. Know the struct to map to the shmem
  2. Save the Handle of the shmem and use it to identify the shmem when you call the API functions
  3. Know how to use the C macro "offsetof" if you want to write only parts of the shmem (https://en.cppreference.com/w/cpp/types/offsetof.html)

Look at the examples for more help in this matter
