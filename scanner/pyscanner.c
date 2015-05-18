#include <stdlib.h>

#include <Python.h>

#include "scanner.h"

static PyObject* pyscanner_list(PyObject *self, PyObject *args)
{
	int number;
	const char **list = scanner_list(&number);
	PyObject *result = PyList_New(number);
	int err = 0;
	int i;

	if (!result) {
		PyErr_NoMemory();
		return NULL;
	}

	for (i = 0; i < number; i++)
		err |= PyList_SetItem(result, i, Py_BuildValue("s", list[i]));

	if (err) {
		PyErr_SetString(PyExc_RuntimeError,
				"Failed to set scanners list");
		Py_CLEAR(result);
		return NULL;
	}

	return result;
}

static void pyscanner_put(PyObject *capsule) {
	struct scanner *scanner = PyCapsule_GetPointer(capsule,
			"pyscanner.pyscanner");

	if (!scanner) {
		PyErr_BadArgument();
		return;
	}

	scanner_put(scanner);
}

static PyObject* pyscanner_get(PyObject *self, PyObject *args)
{
	const char *name;
	struct scanner *scanner;

	if (!PyArg_ParseTuple(args, "s", &name)) {
		PyErr_BadArgument();
		return NULL;
	}

	scanner = scanner_get(name);
	if (!scanner) {
		PyErr_BadArgument();
		return NULL;
	}

	return PyCapsule_New(scanner, "pyscanner.pyscanner", pyscanner_put);
}

static PyObject* pyscanner_on(PyObject *self, PyObject *args)
{
	PyObject *capsule;
	struct scanner *scanner;
	int err;

	if (!PyArg_ParseTuple(args, "O", &capsule)) {
		PyErr_BadArgument();
		return NULL;
	}

	scanner = PyCapsule_GetPointer(capsule, "pyscanner.pyscanner");
	if (!scanner) {
		PyErr_BadArgument();
		return NULL;
	}

	err = scanner_on(scanner);
	if (err) {
		PyErr_SetString(PyExc_RuntimeError,
				"Failed to turn on the scanner");
		return NULL;
	}

	return Py_True;
}

static PyObject* pyscanner_off(PyObject *self, PyObject *args)
{
	PyObject *capsule;
	struct scanner *scanner;

	if (!PyArg_ParseTuple(args, "O", &capsule)) {
		PyErr_BadArgument();
		return NULL;
	}

	scanner = PyCapsule_GetPointer(capsule, "pyscanner.pyscanner");
	if (!scanner) {
		PyErr_BadArgument();
		return NULL;
	}

	scanner_off(scanner);

	return Py_True;
}

static PyObject* pyscanner_get_caps(PyObject *self, PyObject *args)
{
	PyObject *capsule;
	struct scanner *scanner;
	struct scanner_caps caps;
	int err;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "O", &capsule)) {
		PyErr_BadArgument();
		return NULL;
	}

	scanner = PyCapsule_GetPointer(capsule, "pyscanner.pyscanner");
	if (!scanner) {
		PyErr_BadArgument();
		return NULL;
	}

	err = scanner_get_caps(scanner, &caps);
	if (err) {
		PyErr_SetString(PyExc_RuntimeError,
				"Failed to obtain scanner caps");
		return NULL;
	}

	result = PyDict_New();
	if (!result) {
		PyErr_NoMemory();
		return NULL;
	}

	err = 0;
	if (caps.image) {
		switch (caps.image_format) {
		case scanner_image_gray_8bit:
			err |= PyDict_SetItemString(result, "image_format",
					Py_BuildValue("s", "gray 8bit"));
			break;
		case scanner_image_gray_8bit_inversed:
			err |= PyDict_SetItemString(result, "image_format",
					Py_BuildValue("s", "gray 8bit inversed"));
			break;
		default:
			err |= PyDict_SetItemString(result, "unknown",
					Py_BuildValue("s", "gray 8bit"));
			break;
		}
		err |= PyDict_SetItemString(result, "image_width",
				Py_BuildValue("I", caps.image_width));
		err |= PyDict_SetItemString(result, "image_height",
				Py_BuildValue("I", caps.image_height));
	}

	if (caps.iso_template)
		err |= PyDict_SetItemString(result, "iso_template", Py_True);

	if (err) {
		PyErr_SetString(PyExc_RuntimeError,
				"Failed to describe scanner caps");
		Py_CLEAR(result);
		return NULL;
	}

	return result;
}

static PyObject* pyscanner_scan(PyObject *self, PyObject *args)
{
	PyObject *capsule;
	struct scanner *scanner;
	int timeout;
	int err;

	if (!PyArg_ParseTuple(args, "Oi", &capsule, &timeout)) {
		PyErr_BadArgument();
		return NULL;
	}

	scanner = PyCapsule_GetPointer(capsule, "pyscanner.pyscanner");
	if (!scanner) {
		PyErr_BadArgument();
		return NULL;
	}

	err = scanner_scan(scanner, timeout);
	if (err == -1) {
		PyErr_SetString(PyExc_EOFError, "Timeout");
		return NULL;
	} else if (err) {
		PyErr_SetString(PyExc_RuntimeError,
				"Failed to scan");
		return NULL;
	}

	return Py_True;
}

static PyObject* pyscanner_get_image(PyObject *self, PyObject *args)
{
	PyObject *capsule;
	struct scanner *scanner;
	int size;
	int err;
	void *buffer;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "O", &capsule)) {
		PyErr_BadArgument();
		return NULL;
	}

	scanner = PyCapsule_GetPointer(capsule, "pyscanner.pyscanner");
	if (!scanner) {
		PyErr_BadArgument();
		return NULL;
	}

	size = scanner_get_image(scanner, NULL, 0);
	if (size < 0) {
		PyErr_SetString(PyExc_RuntimeError,
				"Failed to check image size");
		return NULL;
	}
	buffer = malloc(size);
	if (!buffer) {
		PyErr_NoMemory();
		return NULL;
	}
	err = scanner_get_image(scanner, buffer, size);
	if (err != size) {
		free(buffer);
		PyErr_SetString(PyExc_RuntimeError, "Failed to get image");
		return NULL;
	}

	result = PyByteArray_FromStringAndSize(buffer, size);
	free(buffer);
	if (!result) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to create image");
		return NULL;
	}

	return result;
}

static PyObject* pyscanner_get_iso_template(PyObject *self, PyObject *args)
{
	PyObject *capsule;
	struct scanner *scanner;
	int size;
	int err;
	void *buffer;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "O", &capsule)) {
		PyErr_BadArgument();
		return NULL;
	}

	scanner = PyCapsule_GetPointer(capsule, "pyscanner.pyscanner");
	if (!scanner) {
		PyErr_BadArgument();
		return NULL;
	}

	size = scanner_get_iso_template(scanner, NULL, 0);
	if (size < 0) {
		PyErr_SetString(PyExc_RuntimeError,
				"Failed to check ISO template size");
		return NULL;
	}
	buffer = malloc(size);
	if (!buffer) {
		PyErr_NoMemory();
		return NULL;
	}
	err = scanner_get_iso_template(scanner, buffer, size);
	if (err != size) {
		free(buffer);
		PyErr_SetString(PyExc_RuntimeError,
				"Failed to get ISO template");
		return NULL;
	}

	result = PyByteArray_FromStringAndSize(buffer, size);
	free(buffer);
	if (!result) {
		PyErr_SetString(PyExc_RuntimeError,
				"Failed to create ISO template");
		return NULL;
	}

	return result;
}

static PyMethodDef pyscanner_methods[] =
{
	{ "list", pyscanner_list, METH_VARARGS, "Returns a list of scanners" },
	{ "get", pyscanner_get, METH_VARARGS, "Obtains a scanner object" },
	{ "on", pyscanner_on, METH_VARARGS, "Turns on a scanner" },
	{ "off", pyscanner_off, METH_VARARGS, "Turns off a scanner" },
	{ "get_caps", pyscanner_get_caps, METH_VARARGS, "Obtains scanner caps" },
	{ "scan", pyscanner_scan, METH_VARARGS, "Performs a single scan" },
	{ "get_image", pyscanner_get_image, METH_VARARGS, "Obtains image" },
	{ "get_iso_template", pyscanner_get_iso_template, METH_VARARGS,
			"Obtains ISO template" },
	{ NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC
initpyscanner(void)
{
	scanner_init();

	(void)Py_InitModule("pyscanner", pyscanner_methods);
}
