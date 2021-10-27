#include "Python.h"

struct module_state {
    PyObject *error;
};

//#define COMPATIBLE
//#define DEBUG_REF_CNTS
#define JOINSTRINGS 1

#ifdef COMPATIBLE
#define TUPLE_SIZE 4
#else
#define TUPLE_SIZE 3
#endif


#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

#define ERROROUT0(S, POS)\
{\
	int 					x, y;\
	struct module_state		*st = GETSTATE(self->self);\
	char 					*lines = searchPosition(self->start, POS, &x, &y);\
	PyErr_Format(st->error, S "\n\nLine %i, column %i:\n%s", y, x, lines);\
	free(lines);\
	Py_XDECREF(children); Py_XDECREF(key); Py_XDECREF(value); Py_XDECREF(res2); Py_XDECREF(attr); return NULL;\
}

#define ERROROUT1(S, POS, S2)\
{\
	int 					x, y;\
	struct module_state		*st = GETSTATE(self->self);\
	char 					*lines = searchPosition(self->start, POS, &x, &y);\
	PyErr_Format(st->error, S "\n\nLine %i, column %i:\n%s", S2, y, x, lines);\
	free(lines); free(S2);\
	Py_XDECREF(children); Py_XDECREF(key); Py_XDECREF(value); Py_XDECREF(res2); Py_XDECREF(attr); return NULL;\
}

#define ERROROUT2(S, POS, S2, S3)\
{\
	int 					x, y;\
	struct module_state		*st = GETSTATE(self->self);\
	char 					*lines = searchPosition(self->start, POS, &x, &y);\
	PyErr_Format(st->error, S "\n\nLine %i, column %i:\n%s", S2, S3, y, x, lines);\
	free(lines); free(S2); free(S3);\
	Py_XDECREF(children); Py_XDECREF(key); Py_XDECREF(value); Py_XDECREF(res2); Py_XDECREF(attr); return NULL;\
}

#define ERROROUTM(S)\
{\
	struct module_state *st = GETSTATE(sself.self);\
	PyErr_SetString(st->error, S);\
	Py_XDECREF(res); return NULL;\
}


struct selfStruct {
	PyObject	*self;
	char		*start;
	int			ExpandEmpty;
};

char *searchPosition(char *start, char *xml, int *x, int *y)
{
	char *line = start;

	*x = 1;
	*y = 1;

	while (start<xml)
	{
		if (*start == '\n')
		{
			(*y)++;
			*x = 1;
			line = start + 1;
		}
		else if (*start == '\r')
		{
			*x = 1;
		}
		else if (*start == '\t')
		{
			*x = ((*x+7)/8)*8 + 1;
		}
		else
			(*x)++;

		start++;
	}

	char* end = line;
	while (*end && *end!='\r' && *end!='\n')
		end++;
	int len = (int)(end-line);
	
	char *output = malloc(len + 1 + *x + 1);
	char *res = output;

	memcpy(output, line, len); output+= len;
	*output++ = '\n';
	memset(output, ' ', *x-1); output+= *x-1;
	*output++ = '^';
	*output = 0;

	return res;
}

char *parse_recurse(struct selfStruct *self, char *xml, PyObject *res, int depth)
{
	PyObject *children = NULL;
	PyObject *key = NULL;
	PyObject *value = NULL;
	PyObject *res2 = NULL;
	PyObject *attr = NULL;

	char *start = NULL;
	char *startb = NULL;
	char *tag = NULL;
	char *end = NULL;

	int len = 0;
	int lenb = 0;
	int lentag = 0;

	int lastWasString = 0;

	while (1)
	{
		// until next tag, collect a text node
		start = xml;
		startb = strchrnul(xml, '>');
		xml = strchrnul(xml, '<');

		// this is only needed to be XML standard compatible. Other XML parsers accept ">" in a text node (e.g. Reportlab pyRXP)
		if (startb!=NULL && startb<xml)
			ERROROUT0("Found \">\" in a text node", startb)

		// we have a text node, add it
		if (xml != start)
		{
			len = (int)(xml-start);

			if (children == NULL)
				children = PyList_New(0);

			if (memchr(start, (int)'&', len) == NULL)
			{
				res2 = PyUnicode_FromStringAndSize(start, len);
			}
			else
			{
				char* copy = malloc(len);
				char* dst = copy;
				int todo = len;

				while (todo>0)
				{
					if (*start=='&')
					{
						if      (todo>3 && strncmp(start+1, "lt;",   3)==0) {
							*dst++ = '<'; start+= 4; todo-= 4; continue;
						}
						else if (todo>3 && strncmp(start+1, "gt;",   3)==0) {
							*dst++ = '>'; start+= 4; todo-= 4; continue;
						}
						else if (todo>5 && strncmp(start+1, "quot;", 5)==0) {
							*dst++ = '"'; start+= 6; todo-= 6; continue;
						}
						else if (todo>4 && strncmp(start+1, "amp;",  4)==0) {
							*dst++ = '&'; start+= 5; todo-= 5; continue;
						}
						else if (todo>5 && strncmp(start+1, "apos;", 5)==0) {
							*dst++ ='\''; start+= 6; todo-= 6; continue;
						}
						else if (todo>2 && *(start+1)=='#')
						{
							// character reference entity
							start+= 2;
							todo-= 2;

							unsigned int valuec = 0;
								
							end = start;
							if (*end=='x')
							{
								// &#xHHHH;
								start++;
								end++;
								todo--;
								while (*end && todo>0)
								{
									if (*end>='0' && *end<='9')
										valuec = (valuec<<4) | (int)(*end-'0');
									else if (*end>='a' && *end<='f')
										valuec = (valuec<<4) | (int)(*end-'a'+10);
									else if (*end>='A' && *end<='F')
										valuec = (valuec<<4) | (int)(*end-'A'+10);
									else
										break;

									end++;
									todo--;
								}
							}
							else
							{
								// &#DD;
								while (*end && *end>='0' && *end<='9' && todo>0)
								{
									valuec = valuec * 10 + (int)(*end-'0');
									end++;
									todo--;
								}
							}

							if (!*end || todo==0)
								ERROROUT0("XML ended while in open character reference entity", start);

							if (start==end || *end!=';')
								ERROROUT0("Invalid character reference entity", start);

							// convert unicode value to utf8 chars
							if (valuec < 0x0080)
							{
								*dst++ = (unsigned char)valuec;
							}
							else if (valuec < 0x0800)
							{
								*dst++ = 0xc0 | (valuec >> 6);
								*dst++ = 0x80 | (valuec & 0x3f);
							}
							else if (valuec >= 0xd800 && valuec <= 0xdfff)
                            {
								ERROROUT0("Invalid UTF-8 codepoint found", start-2);
							}
							else if (valuec < 0x10000)
							{
								*dst++ = 0xe0 |  (valuec >> 12);
								*dst++ = 0x80 | ((valuec >> 6 ) & 0x3f );
								*dst++ = 0x80 |  (valuec        & 0x3f );
							}
							else if (valuec < 0x110000)
							{
								*dst++ = 0xf0 |  (valuec >> 18);
								*dst++ = 0x80 | ((valuec >> 12) & 0x3f);
								*dst++ = 0x80 | ((valuec >>  6) & 0x3f);
								*dst++ = 0x80 | (valuec         & 0x3f);
							}
							else
                            {
								ERROROUT0("Invalid UTF-8 codepoint found", start-2);
							}
							start = end+1;
							todo--;
							continue;
						}
						else
						{
							ERROROUT0("Unknown entity", start);
						}
					}
					
					*dst++ = *start++;
					todo--;
				}

				res2 = PyUnicode_FromStringAndSize(copy, (int)(dst-copy));
				free(copy);
			}

#ifdef JOINSTRINGS
			if (lastWasString)
			{
				int p = PyList_Size(children) - 1;

				PyObject* last = PyList_GetItem(children, p);
				PyObject *joined = PyUnicode_Concat(last, res2);
				PyList_SetItem(children, p, joined);		// steals joined reference
			}
			else
#endif
			{
				PyList_Append(children, res2);

				lastWasString = 1;
			}

			Py_DECREF(res2); res2 = NULL;
		}
	
		// end of XML? bail out
		if (*xml==0)
			break;
		
		// we have previously found a "<" - now check if its closing down or more children
		xml++;

		if (*xml=='/')
		{
			xml--;
			break;
		}
		else if (strncmp(xml, "!--", 3)==0)
		{
			// skip comment
			start = xml - 1;
			xml = strstr(xml+3, "--");

			if (!xml)
				ERROROUT0("XML ended in the middle of a comment, start of comment was here", start);

			if (*(xml+2)!='>')
				ERROROUT0("Found -- within comment, start of comment was here", start);

			xml+= 3;
		}
		else if (strncmp(xml, "![CDATA[", 8)==0)
		{
			start = xml-1;
			xml = strstr(xml+8, "]]>");
			if (!xml)
				ERROROUT0("XML ended in the middle of CDATA, start of CDATA was here", start);

			res2 = PyUnicode_FromStringAndSize(start+8+1, (int)(xml-start-8-1));

			if (lastWasString)
			{
				int p = PyList_Size(children) - 1;

				PyObject* last = PyList_GetItem(children, p);
				PyObject *joined = PyUnicode_Concat(last, res2);
				PyList_SetItem(children, p, joined);		// steals joined reference
			}
			else
			{
				PyList_Append(children, res2);
				lastWasString = 1;
			}

			Py_DECREF(res2); res2 = NULL;

			xml+= 3;
		}
		else if (*xml=='?')
		{
			// skip all <? ... ?> and data before and between
			start = xml - 1;
			xml = strstr(xml+1, "?>");
			if (!xml)
				ERROROUT0("<? found but no ?> found, starting tag was here", start);

			xml+= 2;
		}
		else
		{
			lastWasString = 0;

			// ok, obviously a new tag, so we will add a child
			// parse tag name
			start = xml;
			while (*xml && ((*xml>='a' && *xml<='z') || (*xml>='A' && *xml<='Z') || (start!=xml && ((*xml>='0' && *xml<='9') || *xml==':' || *xml=='_'))))
				xml++;

			if (start == xml)
				ERROROUT0("Expected tag, found nothing", start);

			tag = start;
			lentag = (int)(xml-start);

			if (self->ExpandEmpty)
				attr = PyDict_New();
			else
				attr = NULL;

			int closed = 0;
			while (1)
			{
				// consume spaces
				char *beforeSpaces = xml;
				while (*xml==' ' || *xml=='\n' || *xml=='\r' || *xml=='\t')
					xml++;

				if (!*xml)
					ERROROUT1("End of XML and we are still inside a tag declaration. Last open tag was \"%s\"", xml, strndup(tag, lentag));

				// tag is closing, content coming?
				if (*xml=='>')
				{
					xml++;
					break;
				}

				// directly closed tag "/>" ?
				if (*xml=='/')
				{
					xml++;
					if (*xml=='>')
					{
						xml++;
						closed = 1;
						break;
					}

					ERROROUT0("Expected /> but found only /", xml);
				}

				if (beforeSpaces==xml)
					ERROROUT0("Attributes need a space as divider (or invalid tag name)", xml);

				// tag is not closing, so at least one attribute is coming
				// consume attribute name
				start = xml;
				while (*xml++)
					if (!((*xml>='a' && 'z'>=*xml) || (*xml>='A' && 'Z'>=*xml) || (start!=xml && ((*xml>='0' && *xml<='9') || *xml==':' || *xml=='_'))))
						break;
				len = (int)(xml-start);

				if (!*xml)
					ERROROUT0("End of XML in the middle of an attribute", xml);

				// should never happen
				if (len==0)
					ERROROUT0("Expected attribute, found nothing", xml);

				if (*xml++ != '=')
					ERROROUT0("Expected attribute= but \"=\" was missing", xml-1);

				if (*xml++ != '"')
					ERROROUT0("Expected attr=\" but found attr=", xml-1);

				// get the value inside the "
				startb = xml;
				xml = strchrnul(xml, '"');
				lenb = (int)(xml-startb);
				
				if (!*xml)
					ERROROUT0("Expected attr=\"value\" but found attr=\"value (missing ending quote)", xml);
				xml++; // skip '"'

				// build the key and initialize the attribute dict
				if (attr == NULL)
					attr = PyDict_New();
				key = PyUnicode_FromStringAndSize(start, len);

				// dupe check
				if (PyDict_Contains(attr, key))
					ERROROUT1("Repeated attribute: %s", start, strndup(start, len));

				value = PyUnicode_FromStringAndSize(startb, lenb);

				// set the attribute key: value
				PyDict_SetItem(attr, key, value);

				// clean up
				Py_DECREF(key); key = NULL;
				Py_DECREF(value); value = NULL;
			}

			// create child, recursively fill, append and continue
			res2 = PyTuple_New(TUPLE_SIZE);
		
			PyTuple_SetItem(res2, 0, PyUnicode_FromStringAndSize(tag, lentag));
			if (attr==NULL)
			{
				Py_INCREF(Py_None);
				PyTuple_SetItem(res2, 1, Py_None);
			}
			else
			{
				PyTuple_SetItem(res2, 1, attr);
			}
#ifdef COMPATIBLE
			Py_INCREF(Py_None);
			PyTuple_SetItem(res2, 3, Py_None);
#endif

			if (closed)
			{
				if (self->ExpandEmpty)
					PyTuple_SetItem(res2, 2, PyList_New(0));
				else
				{
					Py_INCREF(Py_None);
					PyTuple_SetItem(res2, 2, Py_None);
				}
			}
			else
			{
				xml = parse_recurse(self, xml, res2, depth+1);
				if (xml==NULL)
					return NULL;

				if (!*xml || *xml!='<')
					ERROROUT1("Expected closing tag, last open tag was \"%s\"", xml, strndup(tag, lentag));
				xml++;

				// this can actually never happen since parse_recurse only exits when its at the end or it find "</"
				if (!*xml || *xml!='/')
					ERROROUT1("Expected closing tag, last open tag was \"%s\"", xml, strndup(tag, lentag));
				xml++;

				start = xml;
				while (*xml && ((*xml>='a' && *xml<='z') || (*xml>='A' && *xml<='Z') || (start!=xml && ((*xml>='0' && *xml<='9') || *xml==':' || *xml=='_'))))
					xml++;
				int len = (int)(xml-start);

				if (len==0)
					ERROROUT1("Ending tag for \"%s\" expected, got nothing", xml, strndup(tag, lentag));

				if (len!=lentag || strncmp(start, tag, len)!=0)
					ERROROUT2("Mismatched end tag: expected </%s>, got </%s>", start, strndup(tag, lentag), strndup(start, len));

				if (!*xml || *xml!='>')
					ERROROUT0("XML ended unexpectedly in a closing tag", xml);
				xml++;
			}

			if (children == NULL)
				children = PyList_New(0);
			PyList_Append(children, res2);
			
			Py_DECREF(res2); res2 = NULL;
		}
	}

	if (children == NULL)
		children = PyList_New(0);
	PyTuple_SetItem(res, 2, children);

	return xml;
}

#ifdef DEBUG_REF_CNTS
void showRefCnts(PyObject *o, int depth)
{
	int i;

	if (o==Py_None)
	{
		printf("        %*sNone\n", depth*4, "");
		return;
	}

	if (PyTuple_Check(o))
	{
		printf("%-8i%*stuple len=%i refcnt=%i addr=%016lx\n", (int)o->ob_refcnt, depth*4, "", (int)PyTuple_Size(o), (int)o->ob_refcnt, (long)o);

		for (i=0; i<PyTuple_Size(o); i++)
		{
			PyObject* c = PyTuple_GetItem(o, i);
			showRefCnts(c, depth+1);
		}
	}
	else if (PyList_Check(o))
	{
		printf("%-8i%*stuple len=%i refcnt=%i addr=%016lx\n", (int)o->ob_refcnt, depth*4, "", (int)PyList_Size(o), (int)o->ob_refcnt, (long)o);

		for (i=0; i<PyList_Size(o); i++)
		{
			PyObject* c = PyList_GetItem(o, i);
			showRefCnts(c, depth+1);
		}
	}
	else if (PyDict_Check(o))
	{
		printf("%-8i%*sdict len=%i refcnt=%i addr=%016lx\n", (int)o->ob_refcnt, depth*4, "", (int)PyDict_Size(o), (int)o->ob_refcnt, (long)o);


		PyObject *key, *value;
		Py_ssize_t pos = 0;

		while (PyDict_Next(o, &pos, &key, &value)) {
			printf("%-8i%*s  key %i addr=%016lx\n", (int)o->ob_refcnt, depth*4, "", (int)pos, (long)o);
			showRefCnts(key, depth+1);
			printf("%-8i%*s  value %i addr=%016lx\n", (int)o->ob_refcnt, depth*4, "", (int)pos, (long)o);
			showRefCnts(value, depth+1);
		}
	}
	else if (PyUnicode_Check(o))
	{
		char* debug = (char*)malloc(PyUnicode_GetSize(o)+1);
		char* s = (char*)PyUnicode_AS_UNICODE(o);
		for (i=0; i<PyUnicode_GetSize(o); i++)
			*(debug+i) = *(s+i*4)>=32 ? (*(s+i*4)) : '.';
		*(debug+PyUnicode_GetSize(o)) = 0;
		printf("%-8i%*sunicode len=%i refcnt=%i value=%s addr=%016lx\n", (int)o->ob_refcnt, depth*4, "", (int)PyUnicode_GetSize(o), (int)o->ob_refcnt, debug, (long)o);
	}
	else
	{
		printf("%-8i%*sUNKNOWN addr=%016lx\n", (int)o->ob_refcnt, depth*4, "", (long)o);
	}
}
#endif

static PyObject *parse(PyObject *self, PyObject *args, PyObject *kwargs)
{
	char					*xml = NULL;
	int						pos = 0;
	struct selfStruct 		sself;

	if (!PyArg_ParseTuple(args, "s", &xml))
		return NULL;

	sself.ExpandEmpty = 0;
	PyObject *ExpandEmpty = PyDict_GetItemString(kwargs, "ExpandEmpty");
	if (ExpandEmpty)
	{
		PyObject *ExpandEmptyNumber = PyNumber_Long(ExpandEmpty);
		if (ExpandEmptyNumber)
		{
			sself.ExpandEmpty = PyLong_AsLong(ExpandEmptyNumber);
			Py_DECREF(ExpandEmptyNumber);
		}
	}

	char* start = xml;

	PyObject *res = PyTuple_New(TUPLE_SIZE);
	Py_INCREF(Py_None);
	PyTuple_SetItem(res, 0, Py_None);
	Py_INCREF(Py_None);
	PyTuple_SetItem(res, 1, Py_None);

#ifdef COMPATIBLE	
	Py_INCREF(Py_None);
	PyTuple_SetItem(res, 3, Py_None);
#endif
	
	sself.start = xml;
	sself.self = self;
	xml = parse_recurse(&sself, xml, res, 0);

	if (xml==NULL)
	{
		Py_DECREF(res);
		return NULL;
	}

	// we only want the children. pop them, inc ref and then kill the rest
	PyObject* found = NULL;
	
	int i;
	PyObject* children = PyTuple_GetItem(res, 2);
	for (i=0; i<PyList_GET_SIZE(children); i++)
	{
		PyObject *child = PyList_GetItem(children, i);

		if (PyTuple_Check(child))
		{
			if (found!=NULL)
				ERROROUTM("Document contains multiple root elements");
			found = child;
		}
		else if (PyUnicode_Check(child))
		{
			// TODO: check if empty, if yes, ignore, if no, bail out
//			printf("got string: %s\n", PyUnicode_AS_DATA(child));
		}
	}

	if (found==NULL)
		ERROROUTM("No XML body found");

	Py_INCREF(found); // found was only borrowed
	Py_DECREF(res);

#ifdef DEBUG_REF_CNTS
	showRefCnts(found, 0);
#endif

	return found;
}

static PyMethodDef speedyxml_methods[] = {
	{"parse",		(PyCFunction)parse,			METH_VARARGS | METH_KEYWORDS,		NULL},
    {NULL, NULL}
};

#if PY_MAJOR_VERSION >= 3

static int speedyxml_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int speedyxml_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "speedyxml",
        NULL,
        sizeof(struct module_state),
        speedyxml_methods,
        NULL,
        speedyxml_traverse,
        speedyxml_clear,
        NULL
};

#define INITERROR return NULL

PyObject *PyInit_speedyxml(void)

#else
#define INITERROR return

void initspeedyxml(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule("speedyxml", speedyxml_methods);
#endif

    if (module == NULL)
        INITERROR;
    struct module_state *st = GETSTATE(module);

    st->error = PyErr_NewException("speedyxml.XMLParseException", NULL, NULL);
    if (st->error == NULL) {
        Py_DECREF(module);
        INITERROR;
    }
	
	PyObject *d = PyModule_GetDict(module);
	PyDict_SetItemString(d, "XMLParseException", st->error);

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
