import random
import os
import sys
from contextlib import contextmanager

PRIMITIVE_TYPES = ["int", "long", "short", "char", "byte", "float", "double", "boolean"]
ARITHMETIC_TYPES = ["int", "long", "short", "byte", "float", "double"]
INTEGRAL_TYPES = ["int", "long", "short", "byte"]
FLOAT_TYPES = ["float", "double"]

INTEGRAL_RANGES = {
    "byte": 2**8,
    "short": 2**16,
    "int": 2**32,
    "long": 2**64
}

MAX_EXPR_DEPTH = 5
MAX_STMT_DEPTH = 3
MAX_FUNC_ARGS = 0
MAX_ARR_SIZE = 5
MAX_STMT_COUNT = 5
MAX_ITERS = 20
MAX_STATIC_VARS = 100
CLASS_NAME = "javafuzz.T1"


def generate_random_identifier(identifier_type, is_function=False):
    identifier_type = identifier_type.replace("[]", "Arr")
    if is_function:
        suff = str(random.randrange(10 ** 5))
        return f"{identifier_type}Fun_{suff}"
    suff = str(random.randrange(10**5))
    return f"{identifier_type}Var_{suff}"


def generate_literal(literal_type):
    if literal_type == "boolean":
        return random.choice(["false", "true"])

    if literal_type == "char":
        return f"(char){random.randrange(2**16)}"

    if literal_type in INTEGRAL_TYPES:
        range_min = -INTEGRAL_RANGES[literal_type] // 2
        range_max = INTEGRAL_RANGES[literal_type] // 2 - 1
        value = str(random.randint(range_min, range_max))
        if literal_type == "long":
            value += "L"
        return value

    if literal_type in FLOAT_TYPES:
        value = str(random.random())
        if literal_type == "float":
            value += "F"
        return value

    raise ValueError(f"Unknown literal type: {literal_type}")


class Identifier:
    def __init__(self, name, type_):
        self.name = name
        self.type = type_
        self.array_size = 0

    def __str__(self):
        return f"{self.type} {self.name}"


class Scope:
    def __init__(self, parent=None):
        self.parent = parent
        self.identifiers = []

    def get_scope(self):
        res = self.identifiers[:]
        if self.parent:
            res += self.parent.get_scope()
        return res

    def fresh_identifier(self, identifier_type, insert=True, is_function=False):
        used = set()
        for i in self.get_scope():
            used.add(i.name)

        name = generate_random_identifier(identifier_type, is_function)
        while name in used:
            name = generate_random_identifier(identifier_type, is_function)

        res = Identifier(name, identifier_type)
        if insert:
            self.identifiers.append(res)
        return res


class Fuzz:
    def __init__(self):
        self.scopes = [Scope()]
        self.newline = False
        self.output = sys.stdout

    def emit(self, *args):
        if self.newline:
            print("  " * len(self.scopes), end="", file=self.output)
            self.newline = False
        print(*args, end=" ", file=self.output)

    def emit_newline(self):
        self.newline = True
        print(file=self.output)

    def start_block(self):
        self.emit("{")
        self.emit_newline()
        self.scopes.append(Scope(self.scopes[-1]))

    def end_block(self):
        self.scopes.pop()
        self.emit("}")
        self.emit_newline()

    @contextmanager
    def block(self):
        self.start_block()
        yield
        self.end_block()

    @contextmanager
    def parentheses(self):
        self.emit("(")
        yield
        self.emit(")")

    def get_scope(self):
        return self.scopes[-1].get_scope()

    def fresh_identifier(self, identifier_type, insert=True, is_function=False):
        return self.scopes[-1].fresh_identifier(identifier_type, insert, is_function)

    def random_type(self):
        is_array = random.random() < 0.3
        base_type = random.choice(PRIMITIVE_TYPES)
        if is_array:
            return f"{base_type}[]"
        return base_type

    def random_var(self, valid_types):
        good = []
        for i in self.get_scope():
            if i.type.replace("[]", "") in valid_types:
                good.append(i)
        if not good:
            return None
        return random.choice(good)

    def random_var_cast(self, valid_type):
        suff = ""
        if valid_type in ARITHMETIC_TYPES:
            t = self.random_var(ARITHMETIC_TYPES)
            if not t:
                return None
            if t.type == valid_type:
                return t.name
            if t.array_size:
                suff = f"[{random.randrange(t.array_size)}]"
            return f"({valid_type}){t.name}{suff}"
        t = self.random_var([valid_type])
        if not t:
            return None
        if t.array_size:
            suff = f"[{random.randrange(t.array_size)}]"
        return t.name + suff

    def function(self, return_type, n_params=None, name=None):
        self.emit("public")
        self.emit("static")
        self.emit(str(return_type))
        if name is None:
            name = self.fresh_identifier(return_type, False, True).name
        self.emit(name)

        if n_params is None:
            n_params = random.randrange(MAX_FUNC_ARGS + 1)

        with self.parentheses():
            for i in range(n_params):
                if i != 0:
                    self.emit(",")
                param_type = self.random_type()
                param = self.fresh_identifier(param_type)
                self.emit(str(param))

        with self.block():
            self.function_body(return_type)

    def function_body(self, return_type):
        self.statements()
        self.return_statement(return_type)

    def statements(self):
        self.declare_var()
        n_statements = random.randint(1, MAX_STMT_COUNT)
        for i in range(n_statements):
            self.statement()

    def declare_var(self, is_static=False):
        var_type = self.random_type()
        var = self.fresh_identifier(var_type, False)
        if is_static:
            self.emit("static")
        self.emit(str(var))
        self.emit("=")
        if "[]" in var_type:
            var.array_size = random.randint(1, MAX_ARR_SIZE)
            self.emit("new")
            self.emit(var_type.replace("[]", ""))
            self.emit("[", str(var.array_size), "]")
        else:
            self.expression(var_type)
        self.emit(";")
        self.emit_newline()
        self.scopes[-1].identifiers.append(var)

    def assign_var(self):
        scope = self.get_scope()
        if not scope:
            return
        lhs = random.choice(scope)
        self.emit(lhs.name)
        if lhs.array_size:
            self.emit("[")
            self.modulo_expression(lhs.array_size)
            self.emit("]")
        self.emit("=")
        self.expression(lhs.type.replace("[]", ""))
        self.emit(";")
        self.emit_newline()

    def if_statement(self):
        has_else = random.random() < 0.5

        self.emit("if")
        with self.parentheses():
            self.expression("boolean")
        with self.block():
            self.statements()
        if has_else:
            self.emit("else")
            with self.block():
                self.statements()

    def for_loop(self):
        iterations = random.randrange(MAX_ITERS)
        first = random.randrange(-2**20, 2**20)
        step = random.randint(1, 1000)
        identifier = self.fresh_identifier("int", insert=False)

        self.emit("for")
        with self.parentheses():
            self.emit(str(identifier))
            self.emit("=", str(first), ";")
            self.emit(identifier.name, "<", first + step * iterations, ";")
            self.emit(identifier.name, "+=", step)

        with self.block():
            # self.scopes[-1].identifiers.append(identifier)
            self.statements()

    def switch_statement(self):
        n = random.randint(1, 10)
        k = random.randint(1, n)
        options = random.sample(list(range(n)), k)

        self.emit("switch")
        with self.parentheses():
            self.modulo_expression(n)

        with self.block():
            for opt in options:
                self.emit("case", str(opt), ":")
                with self.block():
                    self.statements()
                if random.random() > 0.2:
                    self.emit("break", ";")
            self.emit("default", ":")
            with self.block():
                self.statements()

    def statement(self):
        if len(self.scopes) <= MAX_STMT_DEPTH:
            random.choices(population=[
                self.declare_var,
                self.assign_var,
                self.if_statement,
                self.for_loop,
                self.switch_statement
            ])[0]()
        else:
            random.choices(population=[
                self.declare_var,
                self.assign_var,
            ])[0]()

    def expression(self, expr_type, depth=0):
        scope = self.get_scope()
        if depth >= MAX_EXPR_DEPTH or random.random() < 0.5 or expr_type == "char":
            t = self.random_var_cast(expr_type)
            if t:
                self.emit(t)
            else:
                self.emit(generate_literal(expr_type))
            return

        if expr_type == "boolean":
            compare = random.random() < 0.5
            if compare:
                operator = random.choice([">", ">=", "==", "!=", "<", "<="])
                types = random.choice(ARITHMETIC_TYPES)
            else:
                operator = random.choice(["&&", "||"])
                types = "boolean"

            with self.parentheses():
                self.expression(types, depth + 1)
                self.emit(operator)
                self.expression(types, depth + 1)
            return

        if expr_type in ARITHMETIC_TYPES:
            if expr_type in INTEGRAL_TYPES:
                choices = ["&", "|", "~", "^", ">>", "<<", ">>>"]
            else:
                choices = []

            operator = random.choice(["+", "*", "-"] + choices)

            with self.parentheses():
                self.emit(expr_type)
            with self.parentheses():
                if operator == "~":
                    self.emit(operator)
                self.expression(expr_type, depth + 1)
                if operator == "~":
                    return
                self.emit(operator)
                if operator in [">>", "<<", ">>>"]:
                    self.emit(str(random.randrange(5)))
                else:
                    self.expression(expr_type, depth + 1)
            return

        raise ValueError(f"Unknown expression type: {expr_type}")

    def modulo_expression(self, n):
        with self.parentheses():
            with self.parentheses():
                self.expression("int")
            self.emit("%", str(n))
            self.emit("+", str(n))
        self.emit("%", str(n))

    def return_statement(self, return_type):
        self.emit("return")
        self.expression(return_type)
        self.emit(";")
        self.emit_newline()

    def static_vars(self):
        for i in range(random.randrange(MAX_STATIC_VARS + 1)):
            self.declare_var(True)

    def main(self):
        components = CLASS_NAME.split(".")
        os.makedirs(os.path.join(*components[:-1]), exist_ok=True)
        out_file = os.path.join(*components) + ".java"
        with open(out_file, "w") as f:
            self.output = f
            self.emit(f"""
    package {".".join(components[:-1])};
    public class {components[-1]} {{
        public static void main(String[] args) {{
            System.out.println(test());
        }}
    """)
            self.static_vars()
            self.function("int", name="test")
            self.emit("}")


if __name__ == "__main__":
    Fuzz().main()
