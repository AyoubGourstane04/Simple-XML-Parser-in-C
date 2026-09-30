#include "lxml.h"



int main(){

    XMLDocument doc;

    if(XMLDocument_load(&doc, "test.xml")){
        XMLNode* main_node = XMLNode_child(doc.root, 0);
        printf("%d Children\n", main_node->children.size);
        
        XMLNode* description_node = XMLNode_child(main_node, 0);
        printf("%s : %s\n", description_node->tag, description_node->inner_text);

        XMLDocument_free(&doc);
    }

    return 0;
}