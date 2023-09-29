from typing import TypedDict, Optional, Any
import sys

__plpy = sys.modules['__main__'].plpy


class Fields(TypedDict, total=False):
    detail: Optional[str]
    hint: Optional[str]
    sqlstate: Optional[str]
    schema_name: Optional[str]
    table_name: Optional[str]
    column_name: Optional[str]
    datatype_name: Optional[str]
    constraint_name: Optional[str]


def debug(message: Any, **kwargs: Fields):
    __plpy.debug(message, **kwargs)


def log(message: Any, **kwargs: Fields):
    __plpy.log(message, **kwargs)


def info(message: Any, **kwargs: Fields):
    __plpy.info(message, **kwargs)


def notice(message: Any, **kwargs: Fields):
    __plpy.notice(message, **kwargs)


def warning(message: Any, **kwargs: Fields):
    __plpy.warning(message, **kwargs)


def error(message: Any, **kwargs: Fields):
    __plpy.error(message, **kwargs)


def fatal(message: Any, **kwargs: Fields):
    __plpy.fatal(message, **kwargs)
