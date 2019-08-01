#include <Python.h>
#include <yaml.h>


typedef struct node {
    PyObject* value;
    struct node * parent;
} node_t;


node_t* create_node(PyObject* value, node_t*parent) {
    node_t* new_node = malloc(sizeof(node_t));
    new_node->value = value;
    new_node->parent = parent;
    return new_node;
}


static PyObject* handle(PyObject* value, PyObject* parent, PyObject* key) {
    if (PyList_CheckExact(parent)) {
        PyList_Append(parent, value);
    } else if (PyDict_CheckExact(parent)) {
        if (key == NULL) {
            return value;
        } else {
            PyDict_SetItem(parent, key, value);
        }
    }
    return NULL;
}


static PyObject* parse(PyObject* self, PyObject* args)
{
    const char *input;
    int length;

    if (!PyArg_ParseTuple(args, "s#", &input, &length)) {
        return NULL;
    }

    yaml_parser_t parser;
    yaml_event_t event;

    int done = 0;

    yaml_parser_initialize(&parser);
    PyObject* current;
    PyObject* pending_key;
    node_t* current_node = create_node(Py_None, NULL);
    node_t* old_node;

    yaml_parser_set_input_string(&parser,  input, length);

    while (!done) {
        if (!yaml_parser_parse(&parser, &event)) {
            printf("Parser error %d\n", parser.error);
            goto error;
        }

        switch(event.type) {
            case YAML_NO_EVENT: break;
            /* Stream start/end */
            case YAML_STREAM_START_EVENT:  break;
            case YAML_STREAM_END_EVENT: break;
            /* Block delimeters */
            case YAML_DOCUMENT_START_EVENT: break;
            case YAML_DOCUMENT_END_EVENT: break;
            case YAML_SEQUENCE_START_EVENT:
                current = PyList_New(0);
                pending_key = handle(current, current_node->value, pending_key);
                current_node = create_node(current, current_node);
                break;
            case YAML_SEQUENCE_END_EVENT:
                if (current_node->parent->parent != NULL) {
                    old_node = current_node;
                    current_node = current_node->parent;
                    free(old_node);
                }
                break;
            case YAML_MAPPING_START_EVENT:
                current = PyDict_New();
                pending_key = handle(current, current_node->value, pending_key);
                current_node = create_node(current, current_node);
                break;
            case YAML_MAPPING_END_EVENT:
                if (current_node->parent->parent != NULL) {
                    old_node = current_node;
                    current_node = current_node->parent;
                    free(old_node);
                }
                break;
            /* Data */
            case YAML_ALIAS_EVENT:
                break;
            case YAML_SCALAR_EVENT:
                current = PyUnicode_DecodeUTF8(event.data.scalar.value,
                                               event.data.scalar.length,
                                               "strict");
                pending_key = handle(current, current_node->value, pending_key);
                if (current_node->parent == NULL) {
                    current_node->value = current;
                }
                break;
        }

        done = (event.type == YAML_STREAM_END_EVENT);

        yaml_event_delete(&event);

    }

error:
    current = current_node->value;
    free(current_node);
    yaml_parser_delete(&parser);
    return current;
}


static PyMethodDef methods[] = {
    {"parse", parse, METH_VARARGS, "Parse a yaml string"},
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
