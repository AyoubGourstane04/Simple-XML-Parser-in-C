#ifndef LITTLE_XML_H
#define LITTLE_XML_H


#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

/*
    // Definitions
*/

int ends_with(const char* haystack, const char* needle){
    int h_len = strlen(haystack);
    int n_len = strlen(needle);

    if(h_len < n_len) return FALSE;

    for(int i=0; i<n_len; i++){
        if(haystack[h_len - n_len + i] != needle[i]) return FALSE;
    }

    return TRUE;
}



struct _XMLAttribute{
    char* key;
    char* value;
};
typedef struct _XMLAttribute XMLAttribute;

void XMLAttribute_free(XMLAttribute* attr);

struct _XMLAttributeList{
    int heap_size;
    int size;
    XMLAttribute* data;
};
typedef struct _XMLAttributeList XMLAttributeList;

void XMLAttributeList_init(XMLAttributeList* list);
void XMLAttributeList_add(XMLAttributeList* list, XMLAttribute* attr);


struct _XMLNodeList{
    int heap_size;
    int size;
    struct _XMLNode** data;
};
typedef struct _XMLNodeList XMLNodeList;

void XMLNodeList_init(XMLNodeList* list);
void XMLNodeList_add(XMLNodeList* list, struct _XMLNode* node);


struct _XMLNode{
    char* tag;
    char* inner_text;
    struct _XMLNode* parent;
    XMLAttributeList attributes;
    XMLNodeList children;
};
typedef struct _XMLNode XMLNode;

XMLNode* XMLNode_new(XMLNode* parent);
void XMLNode_free(XMLNode* node);
XMLNode* XMLNode_child(XMLNode* parent, int index);
char* XMLNode_attr_val(XMLNode* node, char* key);


struct _XMLDocument{
    XMLNode* root;
    char* version;
    char* encoding;
};
typedef struct _XMLDocument XMLDocument;

int XMLDocument_load(XMLDocument* doc, const char* path);
void XMLDocument_free(XMLDocument* doc);

/*
    // Implementations    
*/

void XMLAttribute_free(XMLAttribute* attr){
    free(attr->key);
    free(attr->value);
}

void XMLAttributeList_init(XMLAttributeList* list){
    list->heap_size = 1;
    list->size = 0;
    list->data = (XMLAttribute*) malloc(sizeof(XMLAttribute) * list->heap_size);
}

void XMLAttributeList_add(XMLAttributeList* list, XMLAttribute* attr){
    while(list->size >= list->heap_size){
        list->heap_size *= 2;
        list->data = (XMLAttribute*) realloc(list->data, sizeof(XMLAttribute) * list->heap_size);
    }

    list->data[list->size++] = *attr;
}


void XMLNodeList_init(XMLNodeList* list){
    list->heap_size = 1;
    list->size = 0;
    list->data = (XMLNode**) malloc(sizeof(XMLNode*) * list->heap_size);
}

void XMLNodeList_add(XMLNodeList* list, XMLNode* node){
    while(list->size >= list->heap_size){
        list->heap_size *= 2;
        list->data = (XMLNode**) realloc(list->data, sizeof(XMLNode*) * list->heap_size);
    }

    list->data[list->size++] = node;
}

XMLNode* XMLNode_new(XMLNode* parent){
    XMLNode* node = (XMLNode*) malloc(sizeof(XMLNode));
    node->parent = parent;
    node->tag = NULL;
    node->inner_text = NULL;
    XMLAttributeList_init(&node->attributes);
    XMLNodeList_init(&node->children);

    if(parent){
        XMLNodeList_add(&parent->children, node);
    }

    return node;
}


void XMLNode_free(XMLNode* node){
    if(node->tag)
        free(node->tag);
    if(node->inner_text)
        free(node->inner_text);

    for(int i=0; i<node->attributes.size; i++){
        XMLAttribute_free(&node->attributes.data[i]);
    }

    for(int i=0; i<node->children.size; i++){
        XMLNode_free(node->children.data[i]);
    }


    free(node);
}

XMLNode* XMLNode_child(XMLNode* parent, int index){
    return parent->children.data[index];
}

char* XMLNode_attr_val(XMLNode* node, char* key){

    for(int i=0; i<node->attributes.size; i++){
        if(!strcmp(node->attributes.data[i].key, key)){
            return node->attributes.data[i].value;
        }
    }

    fprintf(stderr, "No attribute with %s key exists\n", key);
    return NULL;
}

static void parse_attrs(char* buff, int* i, char* lex, int* lexi, XMLNode* curr_node){
    XMLAttribute curr_attr = {0, 0};
    while(buff[*i] != '>'){
        lex[(*lexi)++] = buff[(*i)++];

        // Tag name
        if(buff[*i] == ' ' && !curr_node->tag){
            lex[*lexi] = '\0';
            curr_node->tag = strdup(lex);
            (*lexi) = 0;
            (*i)++;
            continue;
        }

        // Usually ignore space
        if(lex[*lexi-1] == ' '){
            (*lexi)--;
            continue;
        }

        // Attribute key
        if(buff[*i] == '='){
            lex[*lexi] = '\0';
            curr_attr.key = strdup(lex);
            *lexi = 0;
            continue;
        }

        // Attribute value
        if(buff[*i] == '"'){
            if(!curr_attr.key){
                fprintf(stderr, "Value has no key\n");
                return;
            }

            *lexi = 0;
            (*i)++;

            while(buff[*i] != '"'){
                lex[(*lexi)++] = buff[(*i)++];
            }
            lex[*lexi] = '\0';
            curr_attr.value = strdup(lex);
            XMLAttributeList_add(&curr_node->attributes, &curr_attr);
            curr_attr.key = NULL;
            curr_attr.value = NULL;
            *lexi=0;
            (*i)++;
            continue;
        }
    }
}



int XMLDocument_load(XMLDocument* doc, const char* path){

    FILE* file = fopen(path, "r");

    if(!file){
        fprintf(stderr, "Could not open file from '%s' \n", path);
        return FALSE;
    }

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buff = (char*) malloc(sizeof(char) * size + 1);

    fread(buff, 1, size, file);
    fclose(file);

    buff[size] = '\0';

    doc->root = XMLNode_new(NULL);

    char lex[256];
    int lexi = 0;
    int i = 0;

    XMLNode* curr_node = doc->root;

    while(buff[i] != '\0'){
        if(buff[i] == '<'){
            lex[lexi] = '\0';

            // Inner text
            if(lexi > 0){
                if(!curr_node){
                    fprintf(stderr, "Text outside of document\n");
                    return FALSE;
                }

                curr_node->inner_text = strdup(lex);
            
                lexi = 0;
            }

            // End of node
            if(buff[i+1] == '/'){
                i+=2;
                while(buff[i] != '>'){
                    lex[lexi++] = buff[i++];
                }
                lex[lexi] = '\0';

                if(!curr_node){
                    fprintf(stderr, "Already at the root\n");
                    return FALSE;
                }

                if(strcmp(curr_node->tag, lex)){
                    fprintf(stderr, "Mismatched tags (%s != %s)\n", curr_node->tag, lex);
                    return FALSE;
                }

                curr_node = curr_node->parent;
                i++;
                continue;
            }

            // Special nodes
            if(buff[i + 1] == '!'){
                while(buff[i] != ' ' && buff[i] != '>')
                    lex[lexi++] = buff[i++];

                lex[lexi] = '\0';

                // Comments
                if(!strcmp(lex, "<!--")){
                    lex[lexi] = '\0';
                    while(!ends_with(lex, "-->")){
                        lex[lexi++] = buff[i++];
                        lex[lexi] = '\0';
                    }
                    continue;
                }
            }

            // Declaration tags
            if(buff[i + 1] == '?'){
                while(buff[i] != ' ' && buff[i] != '>')
                    lex[lexi++] = buff[i++];

                lex[lexi] = '\0';

                if(!strcmp(lex, "<?xml")){
                    lexi = 0;
                    XMLNode* desc = XMLNode_new(NULL);
                    parse_attrs(buff, &i, lex, &lexi, desc);

                    doc->version = XMLNode_attr_val(desc, "version");
                    doc->encoding = XMLNode_attr_val(desc, "encoding");

                    continue;
                }




            }



            // Set current Node
            curr_node = XMLNode_new(curr_node);

            // Start tag
            i++;
            parse_attrs(buff, &i, lex, &lexi, curr_node);
            
            // Set tag name
            lex[lexi] = '\0';
            if(!curr_node->tag){
                curr_node->tag = strdup(lex);
            }

            // Reset lexer
            lexi = 0;
            i++;
            continue;
        }else{
            lex[lexi++] = buff[i++];
        }



    }







    return TRUE;
}

void XMLDocument_free(XMLDocument* doc){
    free(doc->root);
}




#endif