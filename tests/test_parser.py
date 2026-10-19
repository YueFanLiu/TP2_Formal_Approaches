"""Black-box checks of grammar acceptance, AST structure, and diagnostics."""
from pathlib import Path
import itertools
import subprocess
import sys
import tempfile
import unittest

COMPILER = str(Path(sys.argv.pop(1)).resolve())


def run(source):
    return subprocess.run([COMPILER, "-"], input=source, text=True,
                          capture_output=True, timeout=5)


def tree(label, *children):
    return label + "\n" + "".join(
        "".join("  " + line + "\n" for line in child.splitlines())
        for child in children)


def number(value):
    return tree("Number: " + str(value))


def identifier(name):
    return tree("Identifier: " + name)


def binary(op, left, right):
    return tree("BinaryOp: " + op, left, right)


def assignment(name, value):
    return tree("Assignment", identifier(name), value)


class ParserTests(unittest.TestCase):
    def accepts(self, source, expected=None):
        result = run(source)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        if expected is not None:
            self.assertEqual(result.stdout, expected)
        return result.stdout

    def rejects(self, source, diagnostic):
        result = run(source)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, "", "Do not print a partial AST on failure")
        self.assertIn(diagnostic, result.stderr)
        self.assertIn("Line ", result.stderr)

    def test_precedence_parentheses_and_statement_order(self):
        self.accepts("int a, b; a = 2 + 3 * 4; b = (2 + 3) * 4;",
                     tree("Program",
                          tree("Declaration: int", identifier("a"), identifier("b")),
                          assignment("a", binary("+", number(2), binary("*", number(3), number(4)))),
                          assignment("b", binary("*", binary("+", number(2), number(3)), number(4)))))

    def test_left_associativity(self):
        for op in ("-", "/"):
            with self.subTest(op=op):
                self.accepts(f"a = 20 {op} 5 {op} 2;", tree("Program",
                             assignment("a", binary(op, binary(op, number(20), number(5)), number(2)))))

    def test_mixed_precedence_and_identifiers(self):
        self.accepts("a = b + c / 2 - d * 3;", tree("Program", assignment("a",
                     binary("-", binary("+", identifier("b"), binary("/", identifier("c"), number(2))),
                            binary("*", identifier("d"), number(3))))))

    def test_print_preserves_raw_strings(self):
        self.accepts('print("", "// text", "a\\n", intValue);', tree("Program",
                     tree("Print", tree('String: ""'), tree('String: "// text"'),
                          tree('String: "a\\n"'), identifier("intValue"))))

    def test_nested_blocks_and_else(self):
        self.accepts('if (a > 0) { while (b != 2) { b = b + 1; } } else {}',
                     tree("Program", tree("If",
                          tree("Condition: >", identifier("a"), number(0)),
                          tree("Then", tree("Block", tree("While",
                               tree("Condition: !=", identifier("b"), number(2)),
                               tree("Block", assignment("b", binary("+", identifier("b"), number(1))))))),
                          tree("Else", tree("Block")))))

    def test_switch_order_fallthrough_and_break(self):
        self.accepts('switch (a) { case 0: case 1: print(a); break; default: }',
                     tree("Program", tree("Switch", identifier("a"),
                          tree("Case: 0", tree("Block")),
                          tree("Case: 1", tree("Block", tree("Print", identifier("a")), tree("Break"))),
                          tree("Default", tree("Block")))))

    def test_every_optional_for_component(self):
        for init, cond, update in itertools.product((False, True), repeat=3):
            with self.subTest(init=init, cond=cond, update=update):
                source = "for (" + ("i = 0" if init else "") + ";" + (
                    "i < 5" if cond else "") + ";" + ("i = i + 1" if update else "") + ") {}"
                self.accepts(source, tree("Program", tree("For",
                    tree("Initialization", assignment("i", number(0)) if init else tree("<empty>")),
                    tree("Condition", tree("Condition: <", identifier("i"), number(5)) if cond else tree("<empty>")),
                    tree("Update", assignment("i", binary("+", identifier("i"), number(1))) if update else tree("<empty>")),
                    tree("Block"))))

    def test_valid_grammar_edges(self):
        sources = ["", " \t\r\n// comment at EOF", "int intValue, while2, a_b;",
                   "int a;\r\na = 01; // EOF", "a = (((2)));", "if(a==b){}",
                   "switch(a){}", "switch(a){default:}", "switch(a){case 0: break;}",
                   "switch(a){case 0: switch(b){case 1: break;} break;}",
                   "for(;;){if(a==1){while(b>0){b=b-1;}}}",
                   "// ignored @ and quotes \"\na=1;", 'print("@ # {} ;");']
        for source in sources:
            with self.subTest(source=source):
                self.accepts(source)

    def test_all_comparisons(self):
        for op in (">", "<", ">=", "<=", "==", "!="):
            with self.subTest(op=op):
                self.accepts(f"if ((a + 1) {op} b * 2) {{}}", tree("Program", tree("If",
                    tree("Condition: " + op, binary("+", identifier("a"), number(1)),
                         binary("*", identifier("b"), number(2))), tree("Then", tree("Block")))))

    def test_invalid_grammar(self):
        cases = [
            ("int a", "expected ';'"), ("int;", "expected identifier"),
            ("int a,;", "expected identifier"), ("int if;", "expected identifier"),
            ("int a = 1;", "expected ';'"), ("a = ;", "expected a number"),
            ("a = (1 + 2;", "expected ')'"), ("a = 1 2;", "expected ';'"),
            ("a = -1;", "expected a number"), ("a == 1;", "expected '='"),
            ('a = "x";', "expected a number"), ("a=1;}", "expected a statement"),
            ("print();", "expected a variable or string"),
            ("print(123);", "expected a variable or string"),
            ("print(a + b);", "expected ')'"),
            ('print("x",);', "expected a variable or string"),
            ('print("x")', "expected ';'"),
            ("if (a) {}", "expected comparison operator"),
            ("if (a = 1) {}", "expected comparison operator"),
            ("if (a < b < c) {}", "expected ')'"),
            ("if (a < b) a=1;", "expected '{'"),
            ("if(a<b){", "expected '}' to close block"),
            ("if(a<b){} else if(c<d){}", "expected '{'"),
            ("else {}", "expected a statement"),
            ("while(a<b){a=1;", "expected '}' to close block"),
            ("for(i=0; i<2; i=i+1;){}", "expected ')'"),
            ("for(int i; i<2; i=i+1){}", "expected identifier"),
            ("for(i=0 i<2; i=i+1){}", "expected ';' after for initialization"),
            ("for(;;i++){}", "expected '='"),
            ("for(;1;){}", "expected comparison operator"),
            ("switch(1){}", "expected switch variable"),
            ("switch(a+1){}", "expected ')'"),
            ("switch(a){case x:}", "expected case number"),
            ("switch(a){case -1:}", "expected case number"),
            ("switch(a){case 1}", "expected ':'"),
            ("switch(a){default: case 1:}", "default must be the final branch"),
            ("switch(a){default: default:}", "default must be the final branch"),
            ("switch(a){case 0: break; a=1;}", "after break"),
            ("switch(a){case 0: break}", "expected ';' after break"),
            ("switch(a){case 0:", "expected '}' to close switch"),
            ("break;", "expected a statement"),
            ("while(a<b){break;}", "expected a statement"),
            ("switch(a){case 0: if(a<b){break;}}", "expected a statement"),
            ("case 0:", "expected a statement"),
        ]
        for source, diagnostic in cases:
            with self.subTest(source=source):
                self.rejects(source, diagnostic)

    def test_lexical_failures_and_line_numbers(self):
        for source, diagnostic in [
            ("a=1;\n@", "Line 2: unknown character '@'"),
            ('print("unfinished);', "Line 1: string has no closing quote"),
            ('print("one\ntwo");', "Line 1: string has no closing quote"),
            ("int _a;", "unknown character '_'"),
            ("/* comment */", "expected a statement"),
            ("int a;\r\n\r\na=;", "Line 3:"),
            ("if(a<b){\n", "Line 2:"),
        ]:
            with self.subTest(source=source):
                self.rejects(source, diagnostic)

    def test_file_argument_missing_file_and_usage(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "input with spaces.txt"
            path.write_text("int a;", encoding="utf-8")
            result = subprocess.run([COMPILER, str(path)], capture_output=True, text=True, timeout=5)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout, tree("Program", tree("Declaration: int", identifier("a"))))
            result = subprocess.run([COMPILER, str(path) + ".missing"], capture_output=True, text=True, timeout=5)
            self.assertEqual(result.returncode, 1)
            self.assertIn("Cannot open file", result.stderr)
        result = subprocess.run([COMPILER, "a", "b"], capture_output=True, text=True, timeout=5)
        self.assertEqual(result.returncode, 1)
        self.assertIn("Usage:", result.stderr)

    def test_full_tp1_example(self):
        path = Path(__file__).resolve().parents[1] / "examples" / "tp1_input.txt"
        output = self.accepts(path.read_text(encoding="utf-8"))
        for label in ("If", "Else", "While", "For", "Switch", "Case: 0", "Default", "Break"):
            self.assertIn(label, output)


if __name__ == "__main__":
    unittest.main(verbosity=2)
