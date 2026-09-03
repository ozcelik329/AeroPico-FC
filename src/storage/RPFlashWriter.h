#ifndef RP_FLASH_WRITER_H
#define RP_FLASH_WRITER_H

class RPFlashWriter {
  public:
    using Mutation = void (*)(void*);

    static bool execute(Mutation mutation, void* context);
};

#endif
