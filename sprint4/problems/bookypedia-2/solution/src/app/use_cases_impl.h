#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"
#include "../unit/unit_of_work_factory.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(UnitOfWorkFactory & unit_factory)
        : unit_factory_(unit_factory) {
    }

    void AddAuthor(const std::string& name) override;
    void AddBook(int year, const std::string & author_id, const std::string& title) override;

    authors_list_t GetAuthors() override;
    books_list_t GetBooks() override;
    books_list_t GetBooksAuthors(const std::string & author_id) override;

private:
    UnitOfWorkFactory & unit_factory_;
};

}  // namespace app
