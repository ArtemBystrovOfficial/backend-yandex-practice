#pragma once

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {

class UnitOfWork {
    public:
        virtual void Commit() = 0;
        virtual domain::AuthorRepository & Authors() = 0; 
        virtual domain::BookRepository & Books() = 0;
};

}

