#include "plistlib.h"

HMODULE g_plist_module = NULL;
const char * g_plist_dllname = "libplist.dll";

typedef void     (*plist_from_bin_t)	    (const char *plist_bin, uint32_t length, plist_t * plist);
typedef plist_t  (*plist_new_dict_t)        (void);
typedef uint32_t (*plist_dict_get_size_t)   (plist_t node);
typedef void     (*plist_get_string_val_t)  (plist_t node, char **val);
typedef void     (*plist_get_real_val_t)    (plist_t node, double *val);
typedef plist_t  (*plist_dict_get_item_t)   (plist_t node, const char* key);
typedef void     (*plist_free_t)            (plist_t plist);
typedef void     (*plist_free_string_val_t) (char *val);
typedef void     (*plist_to_xml_t)          (plist_t plist, char **plist_xml, uint32_t * length);
typedef void     (*plist_dict_new_iter_t)   (plist_t node, plist_dict_iter *iter);
typedef void     (*plist_dict_next_item_t)  (plist_t node, plist_dict_iter iter, char **key, plist_t *val);
typedef void	 (*plist_get_data_val_t)    (plist_t node, char **val, uint64_t * length);

typedef uint32_t (*plist_array_get_size_t)	(plist_t node);
typedef plist_t  (*plist_array_get_item_t)	(plist_t node, uint32_t n);
typedef plist_type (*plist_get_node_type_t) (plist_t node);
typedef void	 (*plist_get_uint_val_t)	(plist_t node, uint64_t * val);
typedef void	 (*plist_to_bin_t)			(plist_t plist, char **plist_bin, uint32_t * length);
typedef void	 (*plist_from_xml_t)		(const char *plist_xml, uint32_t length, plist_t * plist);
typedef void     (*plist_get_bool_val_t)	(plist_t node, uint8_t * val);

static plist_from_bin_t f_plist_from_bin = NULL;
static plist_new_dict_t f_plist_new_dict = NULL;
static plist_dict_get_size_t f_plist_dict_get_size = NULL;
static plist_get_string_val_t f_plist_get_string_val = NULL;
static plist_get_real_val_t f_plist_get_real_val = NULL;
static plist_dict_get_item_t f_plist_dict_get_item = NULL;
static plist_free_t f_plist_free = NULL;
static plist_free_string_val_t f_plist_free_string_val = NULL;
static plist_to_xml_t f_plist_to_xml = NULL;
static plist_dict_new_iter_t f_plist_dict_new_iter = NULL;
static plist_dict_next_item_t f_plist_dict_next_item = NULL;
static plist_get_data_val_t f_plist_get_data_val = NULL;

static plist_array_get_size_t f_plist_array_get_size = NULL;
static plist_array_get_item_t f_plist_array_get_item = NULL;
static plist_get_node_type_t f_plist_get_node_type = NULL;
static plist_get_uint_val_t f_plist_get_uint_val = NULL;
static plist_to_bin_t f_plist_to_bin = NULL;
static plist_from_xml_t f_plist_from_xml = NULL;
static plist_get_bool_val_t f_plist_get_bool_val = NULL;

BOOL init_plist_funcs(){
	BOOL ret = FALSE;
	if (!g_plist_module)
	{
		if (g_plist_module = LoadLibraryA(g_plist_dllname))
		{
			f_plist_from_bin = (plist_from_bin_t)GetProcAddress(g_plist_module, "plist_from_bin");
			f_plist_new_dict = (plist_new_dict_t)GetProcAddress(g_plist_module, "plist_new_dict");
			f_plist_dict_get_size = (plist_dict_get_size_t)GetProcAddress(g_plist_module, "plist_dict_get_size");
			f_plist_get_string_val = (plist_get_string_val_t)GetProcAddress(g_plist_module, "plist_get_string_val");
			f_plist_get_real_val = (plist_get_real_val_t)GetProcAddress(g_plist_module, "plist_get_real_val");
			f_plist_dict_get_item = (plist_dict_get_item_t)GetProcAddress(g_plist_module, "plist_dict_get_item");
			f_plist_free = (plist_free_t)GetProcAddress(g_plist_module, "plist_free");
			f_plist_free_string_val = (plist_free_string_val_t)GetProcAddress(g_plist_module, "plist_free_string_val");
			f_plist_to_xml = (plist_to_xml_t)GetProcAddress(g_plist_module, "plist_to_xml");
			f_plist_dict_new_iter = (plist_dict_new_iter_t)GetProcAddress(g_plist_module, "plist_dict_new_iter");
			f_plist_dict_next_item = (plist_dict_next_item_t)GetProcAddress(g_plist_module, "plist_dict_next_item");
			f_plist_get_data_val = (plist_get_data_val_t)GetProcAddress(g_plist_module, "plist_get_data_val");

			f_plist_array_get_size = (plist_array_get_size_t)GetProcAddress(g_plist_module, "plist_array_get_size");
			f_plist_array_get_item = (plist_array_get_item_t)GetProcAddress(g_plist_module, "plist_array_get_item");
			f_plist_get_node_type = (plist_get_node_type_t)GetProcAddress(g_plist_module, "plist_get_node_type");
			f_plist_get_uint_val = (plist_get_uint_val_t)GetProcAddress(g_plist_module, "plist_get_uint_val");
			f_plist_to_bin = (plist_to_bin_t)GetProcAddress(g_plist_module, "plist_to_bin");
			f_plist_from_xml = (plist_from_xml_t)GetProcAddress(g_plist_module, "plist_from_xml");
			f_plist_get_bool_val = (plist_get_bool_val_t)GetProcAddress(g_plist_module, "plist_get_bool_val");
			ret = TRUE;
		}			 
	} else {
		ret = TRUE;
	}
	return ret;
}

PLIST_API void plist_from_bin(const char *plist_bin, uint32_t length, plist_t * plist)
{
	f_plist_from_bin(plist_bin, length, plist);
}

PLIST_API plist_t plist_new_dict(void)
{
	return f_plist_new_dict();
}

PLIST_API uint32_t plist_dict_get_size(plist_t node)
{
	return f_plist_dict_get_size(node);
}

PLIST_API void plist_get_string_val(plist_t node, char **val)
{
	f_plist_get_string_val(node, val);
}

PLIST_API void plist_get_real_val(plist_t node, double *val)
{
	f_plist_get_real_val(node, val);
}

PLIST_API plist_t plist_dict_get_item(plist_t node, const char* key)
{
	return f_plist_dict_get_item(node, key);
}

PLIST_API void plist_free(plist_t plist)
{
	f_plist_free(plist);
}

PLIST_API void plist_free_string_val(char *val)
{
	f_plist_free_string_val(val);
}

PLIST_API void plist_to_xml(plist_t plist, char **plist_xml, uint32_t * length)
{
	f_plist_to_xml(plist, plist_xml, length);
}

PLIST_API void plist_dict_new_iter(plist_t node, plist_dict_iter *iter)
{
	f_plist_dict_new_iter(node, iter);
}

PLIST_API void plist_dict_next_item(plist_t node, plist_dict_iter iter, char **key, plist_t *val)
{
	f_plist_dict_next_item(node, iter, key, val);
}

 PLIST_API void plist_get_data_val(plist_t node, char **val, uint64_t * length)
 {
	f_plist_get_data_val(node, val, length);
 }

PLIST_API uint32_t plist_array_get_size(plist_t node)
{
	return f_plist_array_get_size(node);
}

PLIST_API plist_t plist_array_get_item(plist_t node, uint32_t n)
{
	return f_plist_array_get_item(node, n);
}

PLIST_API plist_type plist_get_node_type(plist_t node)
{
	return f_plist_get_node_type(node);
}

PLIST_API void plist_get_uint_val(plist_t node, uint64_t * val)
{
	return f_plist_get_uint_val(node, val);
}

PLIST_API void plist_to_bin(plist_t plist, char **plist_bin, uint32_t * length)
{
	f_plist_to_bin(plist, plist_bin, length);
}

PLIST_API void plist_from_xml(const char *plist_xml, uint32_t length, plist_t * plist)
{
	f_plist_from_xml(plist_xml, length, plist);
}

PLIST_API void plist_get_bool_val(plist_t node, uint8_t * val)
{
	f_plist_get_bool_val(node, val);
}