#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(int year, const std::string & author_id, const std::string& title) {
    books_.Save({BookId::New(), AuthorId::FromString(author_id), title, year });
}

UseCases::authors_list_t UseCasesImpl::GetAuthors() { 
    auto authors_list = authors_.GetList();
    authors_list_t authors_list_case;
    std::transform(authors_list.begin(), authors_list.end(),std::back_inserter(authors_list_case),
    [](const Author & author) -> detail::AuthorInfo {
        return detail::AuthorInfo{author.GetId().ToString(), author.GetName()};
    });
    return authors_list_case;
}

UseCases::books_list_t UseCasesImpl::GetBooks() {
    auto books_list = books_.GetList();
    books_list_t books_list_case;
    std::transform(books_list.begin(), books_list.end(),std::back_inserter(books_list_case),
    [](const Book & book) -> detail::BookInfo {
        return detail::BookInfo{book.GetTitle(),book.GetYear()};
    });
    return books_list_case;
}

UseCases::books_list_t UseCasesImpl::GetBooksAuthors(const std::string & author_id) {
    auto books_list = books_.GetBookByAuthorId(AuthorId::FromString(author_id));
    books_list_t books_list_case;
    std::transform(books_list.begin(), books_list.end(),std::back_inserter(books_list_case),
    [](const Book & book) -> detail::BookInfo {
        return detail::BookInfo{book.GetTitle(),book.GetYear()};
    });
    return books_list_case;
}

}  // namespace app
