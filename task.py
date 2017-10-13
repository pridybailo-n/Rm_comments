"""
    File name: task.py
    Author: Nikolay Pidybailo
    Python Version: 3.5
"""
import re


def rmcomments(source, dest):
    """ Removes C++ style comments from file.
        source: path to soure file
        dest: path to destination file
    """
    with open(source) as file:
        pattern = re.compile(r'//.*?$|/\*.*?\*/', re.DOTALL | re.MULTILINE)
        nocmt = re.sub(pattern, '', file.read())

    with open(dest, 'w') as file:
        file.write(nocmt)


rmcomments("test/input.c", "test/output.c")
