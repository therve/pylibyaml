#include <Python.h>

static PyObject* parse(PyObject* self, PyObject* args)
{
    printf("parse\n");
    return Py_None;
}

static PyMethodDef methods[] = {
    {"parse", parse, METH_NOARGS, "Parse a yaml string"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "_ext",
    "libyaml extension module",
    -1,
    methods
};

PyMODINIT_FUNC PyInit__ext(void)
{
    return PyModule_Create(&module);
}
