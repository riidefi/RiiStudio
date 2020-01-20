import sys
import riistudio

# Based on the blender script



def add_scrollback(text, text_type):
    for l in text.split("\n"):
        riistudio.get_console_handle().scrollback_append(l.replace("\t", "    "), text_type)

def replace_help(namespace):
    def _help(*args):
        import pydoc
        pydoc.getpager = lambda : pydoc.plainpager
        pydoc.Helper.getline = lambda self, prompt : None
        pydoc.TextDoc.use_bold = lambda self, text : text
        pydoc.help(*args)

    namespace["help"] = _help

def get_console():
    from code import InteractiveConsole

    
    namespace = {}
    namespace["__builtins__"] = sys.modules["builtins"]
    namespace["riistudio"] = riistudio
    namespace["rs"] = riistudio
    replace_help(namespace)
    console = InteractiveConsole(locals=namespace, filename='<riistudio_console>')

    

    #riistudio.set_console_handle(console, stdout, stderr)
    return console

console = get_console()
    
PROMPT = '>>> '
PROMPT_MULTI = '... '


def execute(is_interactive=True):

    line_object = riistudio.get_console_handle().get_history_last()
    
    import io
    stdout = io.StringIO()
    stderr = io.StringIO()

    from contextlib import (
        redirect_stdout,
        redirect_stderr
    )

    class redirect_stdin(redirect_stdout.__base__):
        _stream = "stdin"

    with redirect_stdout(stdout), redirect_stderr(stderr), redirect_stdin(None):
        line = ""
        is_multiline = False

        try:
            line = line_object.body
            line_exec = line if line.strip() else "\n"
            is_multiline = console.push(line_exec)
        except:
            import traceback
            stderr.write(traceback.format_exc())
        
    stdout.seek(0)
    stderr.seek(0)

    output = stdout.read()
    output_err = stderr.read()

    sys.last_traceback = None

    stdout.truncate(0)
    stderr.truncate(0)

    riistudio.get_console_handle().scrollback_append(riistudio.get_console_handle().prompt + line, 'INPUT')

    if is_multiline:
        riistudio.get_console_handle().prompt = PROMPT_MULTI
        if is_interactive:
            indent = line[:len(line) - len(line.lstrip())]
            if line.rstrip().endswith(":"):
                indent += "    "
        else:
            indent = ""
    else:
        riistudio.get_console_handle().prompt = PROMPT
        indent = ""

    # insert a new blank line
    #bpy.ops.console.history_append(text=indent, current_character=0,
    #                               remove_duplicates=True)
    h = riistudio.get_console_handle().get_history_last()
    h.current_character = len(indent)
    riistudio.get_console_handle().set_history_last(h)

    # Insert the output into the editor
    # not quite correct because the order might have changed,
    # but ok 99% of the time.
    if output:
        add_scrollback(output, 'OUTPUT')
    if output_err:
        add_scrollback(output_err, 'ERROR')

    