cmake_minimum_required(VERSION 3.25)
project(py_omni_python)

include(../PythonPackage.cmake)

add_python_package(py_omni_python
        PACKAGE_NAME omni_python
        VERSION 0.1.0
        SOURCES
        src/omni_python/__init__.py
        src/omni_python/log.py
	src/omni_python/dbapi.py
        )
