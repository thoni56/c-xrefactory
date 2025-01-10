import gdb
import re

# Globala variabler för typmappning
yacc_types = {}

class YaccCommand(gdb.Command):
    """Kommandot 'yacc': hanterar semantiska attribut i Yacc"""

    def __init__(self):
        super(YaccCommand, self).__init__("yacc", gdb.COMMAND_USER)
        gdb.write("YaccCommand registered as 'yacc'\n")

    def invoke(self, arg, from_tty):
        global yacc_types
        args = arg.split()
        if len(args) < 2:
            gdb.write("Usage: yacc <n> <symbol> [field]\n", gdb.STDERR)
            return

        index = int(args[0])  # Index i parsningstacken
        symbol = args[1]      # Namn på symbol
        field = args[2] if len(args) > 2 else None  # Valfritt fält

        # Hitta typen för symbolen
        if symbol not in yacc_types:
            gdb.write(f"Symbol '{symbol}' not found in types\n", gdb.STDERR)
            return
        yacc_type = yacc_types[symbol]

        # Hantera specialfallet för $$
        if index == 0:
            expression = f"c_yyval.{yacc_type}"
            if field:
                expression += f".{field}"

            gdb.write(f"Evaluating expression: {expression}\n")
            try:
                value = gdb.parse_and_eval(expression)
                gdb.write(f"Value (for $$): {value}\n")
            except gdb.error as e:
                gdb.write(f"GDB error evaluating expression '{expression}': {e}\n", gdb.STDERR)
            return

        # Hämta regellängden (yym)
        try:
            yym = int(gdb.parse_and_eval("yym"))  # Hämta regellängden
            gdb.write(f"Rule length (yym): {yym}\n")
        except gdb.error as e:
            gdb.write(f"Failed to retrieve yym: {e}\n", gdb.STDERR)
            return

        # Kontrollera om yym är 0
        if yym == 0:
            gdb.write("Error: Rule length (yym) is 0, likely due to an embedded semantic action.\n", gdb.STDERR)
            return

        # Beräkna stackens index
        stack_index = -yym + index
        expression = f"c_yyvsp[{stack_index}].{yacc_type}"

        # Lägg till fält om det anges
        if field:
            expression += f".{field}"

        gdb.write(f"Evaluating expression: {expression}\n")

        # Evaluera uttrycket
        try:
            value = gdb.parse_and_eval(expression)

            # Kontrollera och avreferera pekare
            if field and value.type.code == gdb.TYPE_CODE_PTR:
                value = value.dereference()

            gdb.write(f"Value: {value}\n")
        except gdb.error as e:
            gdb.write(f"GDB error evaluating expression '{expression}': {e}\n", gdb.STDERR)


# Registrera kommandot
YaccCommand()

def find_project_root():
    """Hitta projektroten genom att söka efter '.git' i katalogstrukturen."""
    dir_path = os.getcwd()
    while dir_path != '/':
        if os.path.exists(os.path.join(dir_path, '.git')):
            return dir_path
        dir_path = os.path.dirname(dir_path)
    raise RuntimeError("Could not find project root (no .git directory found)")

def load_yacc_types(y_file):
    """Läser en .y-fil och bygger en mappning av typer"""
    global yacc_types
    with open(y_file, 'r') as f:
        for line in f:
            # Matcha `%type <typ> symboler`
            match = re.match(r'%type\s*<(\w+)>\s*(.+)', line)
            if match:
                yacc_type = match.group(1)
                symbols = match.group(2).split()
                for symbol in symbols:
                    yacc_types[symbol] = yacc_type
    gdb.write(f"Loaded types from {y_file}: {len(yacc_types)} symbols\n")

def initialize_yacc_types():
    """Hitta projektroten och ladda Yacc-typer från src/c_parser.y."""
    try:
        project_root = find_project_root()
        yacc_file_path = os.path.join(project_root, 'src', 'c_parser.y')
        load_yacc_types(yacc_file_path)
    except Exception as e:
        gdb.write(f"Failed to load Yacc types: {e}\n", gdb.STDERR)

initialize_yacc_types()
