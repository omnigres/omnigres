from omni_python import pg

import another_module
import module1
import module1.submod

@pg
def fun1(v: str) -> int:
    return len(v)


@pg
def fun2(v: str) -> str:
    return v[::-1]


@pg
def fun3() -> str:
    return another_module.impl()


@pg
def fun4() -> str:
    return module1.impl()


@pg
def fun5() -> str:
    return module1.submod.impl()


@pg
def add(v1: str, v2: str) -> str:
    from arithmetics import arithmetics
    return arithmetics.calculate(arithmetics.OPERAND_ADD, v1, v2)