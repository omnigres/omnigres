import typing
from typing import Any, TypedDict, Optional
from dataclasses import dataclass


@dataclass
class pgtype:
    name: str


def Composite(klass: Any, name: str):
    """
    Postgres composite type hint
    :param klass: Typed dictionary Python class
    :param name: The name of the type in Postgres
    :return:
    """
    return typing.Annotated[klass, pgtype(name), "composite"]


def Custom(klass: Any, name: str):
    """
    Postgres composite type hint
    :param klass: Typed dictionary Python class
    :param name: The name of the type in Postgres
    :return:
    """
    return typing.Annotated[klass, pgtype(name)]


class PgAttributes(TypedDict, total=False):
    name: Optional[str]


def pg(*args, **attrs: PgAttributes):
    """
    Decorator that annotates the function to be available as a Postgres stored procedure

    :param attrs: Function attributes
    :return: decorator
    """

    def pg_fun(fun):
        fun.__pg_stored_procedure__ = attrs
        return fun

    if len(args) == 1 and callable(args[0]) and not attrs:
        return pg_fun(args[0])
    else:
        return pg_fun
