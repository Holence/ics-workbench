import gdb


def on_stop(_):
    print("")
    gdb.execute("x/16xg $rsp")
    gdb.execute("p/x jmpbuf")


gdb.events.stop.connect(on_stop)
gdb.execute("file ./asm-64")
gdb.Breakpoint("main")
gdb.execute("layout split")
gdb.execute("run")

# enable Reverse Execution
gdb.execute("target record-full")
