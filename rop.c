#include "rop.h"

int rop_chain(unsigned char *binary, unsigned long binary_len)
{
    struct Gadget *head;
    head = (struct Gadget *)malloc(sizeof(struct Gadget));
    if(!head)
    {
        fprintf(stderr ,"malloc failed.\n");
        return -1;
    }
    rop_chain_list_init(head);
    rop_find_gadgets("pop", "eax", head, binary, binary_len);
    rop_find_gadgets("pop", "ebx", head, binary, binary_len);
    rop_find_gadgets("pop", "ecx", head, binary, binary_len);
    rop_find_gadgets("pop", "edx", head, binary, binary_len);
    rop_find_gadgets("xor", "eax, eax", head, binary, binary_len);
    rop_find_gadgets("int", "0x80", head, binary, binary_len);

    rop_chain_list_traverse(head);
    return 0;
}

int rop_find_gadgets(char* operate, char* operand, struct Gadget *head, unsigned char *binary, unsigned long binary_len)
{
    size_t count;
    csh handle;
    cs_insn *insn;
    struct Gadget temp;
    size_t i,j;
    unsigned int text_address = 0x08048000;

    if(cs_open(CS_ARCH_X86, CS_MODE_32, &handle) != CS_ERR_OK)
    {
        return -1;
    }
    for(i = 0; i < binary_len - MaxGadgetByte; i++)
    {
        count = cs_disasm_ex(handle, binary + i, MaxGadgetByte, text_address + i, 0, &insn);
        if(count > 0)
        {
            strcpy(temp.string, "");
            for(j = 0; j < count; j++)
            {
                if(!strcmp(insn[j].mnemonic, "ret") && \
                (!strcmp(insn[j-1].mnemonic, operate) || !strcmp(operate, "xxx"))&& \
                (!strcmp(insn[j-1].op_str, operand) || !strcmp(operand, "xxx")))
                {
                    strcat(temp.string, insn[j-1].mnemonic);
                    strcat(temp.string, " ");
                    strcat(temp.string, insn[j-1].op_str);
                    strcat(temp.string, " ; ");
                    temp.address = insn[j-1].address;
                    strcat(temp.string, "ret");

                    rop_chain_list_add(head, temp.address, temp.string);
                    strcpy(temp.string, "");
                    cs_close(&handle);
                    cs_free(insn, count);
                    return 0;
                }
            }
        }
    }
    printf("-x--------: Can't find *%s %s ; ret*\n", operate, operand);
    cs_close(&handle);
    cs_free(insn, count);
    return -1;
}


void rop_chain_list_init(struct Gadget *head)
{
    head->next = 0;
    head->prev = 0;
}

int rop_chain_list_add(struct Gadget *head, unsigned int address, char *string)
{
    struct Gadget *gadget;
    if(strlen(string) > MaxGadgetLen)
    {
        fprintf(stderr ,"Gadget buffer overflow.\n");
        return -1;
    }

    gadget = (struct Gadget *)malloc(sizeof(struct Gadget));
    if(!gadget)
    {
        fprintf(stderr ,"malloc failed.\n");
        exit(-1);
    }
    gadget->address = address;
    gadget->next = NULL;
    strcpy(gadget->string, string);
    if(head->next)
    {
        gadget->prev = head->prev;
        head->prev->next = gadget;
        head->prev = gadget;
    }
    else
    {
        head->next = gadget;
        head->prev = gadget;
    }
    return 0;
}

void rop_chain_list_traverse(struct Gadget *head)
{
    struct Gadget *temp;
    for(temp = head->next; temp; temp = temp->next)
    {
        printf("0x0%x: %s\n", temp->address, temp->string);
    }

}
