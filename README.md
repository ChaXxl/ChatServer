~~~
├── bin
│   └── ChatServer
├── build
├── CMakeLists.txt
├── include
│   ├── public.hpp
│   └── server
│       ├── chatserver.hpp
│       ├── chatservice.hpp
│       ├── db
│       │   └── db.h
│       └── model
│           ├── friendmodel.h
│           ├── group.hpp
│           ├── groupmodel.hpp
│           ├── groupuser.hpp
│           ├── offlinemessagemodel.h
│           ├── user.hpp
│           └── usermodel.h
├── run.sh
├── src
│   ├── client
│   ├── CMakeLists.txt
│   └── server
│       ├── chatserver.cpp
│       ├── chatservice.cpp
│       ├── CMakeLists.txt
│       ├── db
│       │   └── db.cpp
│       ├── main.cpp
│       └── model
│           ├── friendmodel.cpp
│           ├── groupmodel.cpp
│           ├── offlinemessagemodel.cpp
│           └── usermodel.cpp
└── thirdparty
    └── json.hpp
~~~


# 编译
~~~shell
cd build
cmake ..
make
~~~


# 运行服务端

~~~shell
~~~



# 运行客户端

~~~shell
telnet 127.0.0.1 8888
~~~





