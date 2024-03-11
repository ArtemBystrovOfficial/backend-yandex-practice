#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
        : authors_{authors}
        , books_(books) {
    }

    void AddAuthor(const std::string& name) override;
    void AddBook(int year, const std::string & author_id, const std::string& title) override;

    authors_list_t GetAuthors() override;
    books_list_t GetBooks() override;
    books_list_t GetBooksAuthors(const std::string & author_id) override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app
