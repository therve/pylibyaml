#include <Python.h>
#include <yaml.h>

static PyObject *ParserError;


#define HANDLE switch(handle(current, current_node->value, current_node->pending_key)) {\
                    case -1:\
                        current_node->value = NULL;\
                        yaml_event_delete(&event);\
                        goto error;\
                    case 1:\
                        current_node->pending_key = current;\
                        break;\
                    case 0:\
                        current_node->pending_key = NULL;\
                        break;\
                }


typedef struct node {
    PyObject* value;
    struct node* parent;
    PyObject* pending_key;
} node_t;


node_t* create_node(PyObject* value, node_t* parent, PyObject* key) {
    node_t* new_node = PyMem_RawMalloc(sizeof(node_t));
    new_node->value = value;
    new_node->parent = parent;
    new_node->pending_key = key;
    return new_node;
}


static int handle(PyObject* value, PyObject* parent, PyObject* key) {
    if (value == NULL) {
        return -1;
    }
    if (PyList_CheckExact(parent)) {
        if (PyList_Append(parent, value) == -1) {
            return -1;
        }
    } else if (PyDict_CheckExact(parent)) {
        if (key == NULL) {
            return 1;
        } else {
            if (PyDict_SetItem(parent, key, value) == -1) {
                return -1;
            };
        }
    }
    return 0;
}

char *make_message(const char *fmt, ...){
    int n;
    int size = 100;
    char *p, *np;
    va_list ap;

    if ((p = PyMem_RawMalloc(size)) == NULL) {
        return NULL;
    }

    while (1) {
        /* Try to print in the allocated space */
        va_start(ap, fmt);
        n = vsnprintf(p, size, fmt, ap);
        va_end(ap);

        /* Check error code */

        if (n < 0) {
            return NULL;
        }

        /* If that worked, return the string */

        if (n < size) {
            return p;
        }

        /* Else try again with more space */

        size = n + 1;

        if ((np = PyMem_RawRealloc(p, size)) == NULL) {
            PyMem_RawFree(p);
            return NULL;
        } else {
            p = np;
        }
    }
}


static PyObject* parse(PyObject* self, PyObject* args)
{
    const unsigned char *input;
    int length;

    if (!PyArg_ParseTuple(args, "s#", &input, &length)) {
        return NULL;
    }

    yaml_parser_t parser;
    yaml_event_t event;

    int done = 0;

    yaml_parser_initialize(&parser);
    PyObject* current;
    node_t* current_node = create_node(Py_None, NULL, NULL);
    node_t* old_node;

    yaml_parser_set_input_string(&parser,  input, length);

    while (!done) {
        if (!yaml_parser_parse(&parser, &event)) {
            const char* message;
            switch(parser.error) {
                case YAML_MEMORY_ERROR:
                    message = "Memory error";
                    break;
                case YAML_READER_ERROR:
                    message = parser.problem;
                    break;
                case YAML_SCANNER_ERROR:
                case YAML_PARSER_ERROR:
                    // column doesn't seem to have proper information
                    message = make_message("%s %s, line %d", parser.context, parser.problem, parser.context_mark.line);
                    break;
                default:
                    message = "Parsing failed";
            }
            PyErr_SetString(ParserError, message);
            current_node->value = NULL;
            goto error;
        }

        switch(event.type) {
            case YAML_NO_EVENT:
                break;
            /* Stream start/end */
            case YAML_STREAM_START_EVENT:
                break;
            case YAML_STREAM_END_EVENT:
                break;
            /* Block delimeters */
            case YAML_DOCUMENT_START_EVENT:
                break;
            case YAML_DOCUMENT_END_EVENT:
                break;
            case YAML_SEQUENCE_START_EVENT:
                current = PyList_New(0);
                HANDLE;
                current_node = create_node(current, current_node, current_node->pending_key);
                break;
            case YAML_SEQUENCE_END_EVENT:
                if (current_node->parent->parent != NULL) {
                    old_node = current_node;
                    current_node = current_node->parent;
                    PyMem_RawFree(old_node);
                }
                break;
            case YAML_MAPPING_START_EVENT:
                current = PyDict_New();
                HANDLE;
                current_node = create_node(current, current_node, current_node->pending_key);
                break;
            case YAML_MAPPING_END_EVENT:
                if (current_node->parent->parent != NULL) {
                    old_node = current_node;
                    current_node = current_node->parent;
                    PyMem_RawFree(old_node);
                }
                break;
            /* Data */
            case YAML_ALIAS_EVENT:
                break;
            case YAML_SCALAR_EVENT:
                current = PyUnicode_DecodeUTF8((const char*) event.data.scalar.value,
                                               event.data.scalar.length,
                                               "strict");
                HANDLE;
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
    PyMem_RawFree(current_node);
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
    methods,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC PyInit__ext(void) {
    PyObject *m;
    m = PyModule_Create(&module);
    if (m == NULL) {
        return NULL;
    }

    if (ParserError == NULL) {
        ParserError = PyErr_NewException("_ext.ParserError", NULL, NULL);
    }

    if (ParserError != NULL) {
        Py_INCREF(ParserError);
        PyModule_AddObject(m, "ParserError", ParserError);
    }

    return m;
}
