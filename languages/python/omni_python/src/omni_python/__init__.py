import typing
from typing import Any
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


def pg(fun):
    """
    Decorator that annotates the function to be available as a Postgres stored procedure

    :param fun: Function to be decorated
    :return: same function
    """

    fun.__pg_stored_procedure__ = True
    return fun
