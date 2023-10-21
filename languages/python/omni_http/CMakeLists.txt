cmake_minimum_required(VERSION 3.25)
project(py_omni_http)

include(../PythonPackage.cmake)

add_python_package(py_omni_http
        PACKAGE_NAME omni_http
        VERSION 0.1.0
        SOURCES
        src/omni_http/__init__.py
        src/omni_http/omni_http.py
        src/omni_http/omni_httpd/__init__.py
        src/omni_http/omni_httpd/flask.py
        )