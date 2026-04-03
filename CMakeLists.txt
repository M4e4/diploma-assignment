cmake_minimum_required(VERSION 3.16)
project(ProjectName)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    main.cpp 
	"source/indexer.cpp"
    "source/database.cpp" 
    "source/iniParser.cpp"
    "source/threadPool.cpp"
    "source/crawler.cpp"
    "source/httpClient.cpp"
    "source/httpParser.cpp"
    "source/urlParser.cpp"
    "source/httpServer.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/source
)

find_package(Boost REQUIRED COMPONENTS system thread regex locale filesystem)
find_package(libpqxx CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::system
    Boost::thread
    Boost::regex
    Boost::locale
    Boost::filesystem
    libpqxx::pqxx
    OpenSSL::SSL OpenSSL::Crypto
)