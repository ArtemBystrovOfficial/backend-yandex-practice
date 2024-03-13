#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::EditAuthorName(const std::string& author_id,const std::string& author_new_name) {
    CreateNextUnitWork();
    last_unit_of_work_->Authors().Save({domain::AuthorId::FromString(author_id), author_new_name});
}

void UseCasesImpl::DeleteAuthorAndDependenciesByName(const std::string& author_name) {
    CreateNextUnitWork();
    last_unit_of_work_->Authors().DeleteAuthorAndDependencies({{}, author_name});
}

void UseCasesImpl::DeleteAuthorAndDependencies(const std::string& author_id) {
    CreateNextUnitWork();
    last_unit_of_work_->Authors().DeleteAuthorAndDependencies({domain::AuthorId::FromString(author_id), ""});
}

std::string UseCasesImpl::AddAuthor(const std::string& name) {
    CreateNextUnitWork();
    auto id = AuthorId::New();
    last_unit_of_work_->Authors().Save({id, name});
    return id.ToString();
}

std::string UseCasesImpl::AddBook(int year, const std::string & author_id, const std::string& title) {
    CreateNextUnitWork();
    auto id = BookId::New();
    last_unit_of_work_->Books().Save({id, {AuthorId::FromString(author_id),""}, title, year });
    return id.ToString();
}

void UseCasesImpl::AddTags(const std::string& book_id, const std::vector<std::string>& tags) {
    CreateNextUnitWork();
    for(const auto & tag : tags)
        last_unit_of_work_->Tags().Save({domain::BookId::FromString(book_id), tag});
}

UseCases::authors_list_t UseCasesImpl::GetAuthors() { 
    CreateNextUnitWork();

    auto authors_list = last_unit_of_work_->Authors().GetList();
    authors_list_t authors_list_case;
    std::transform(authors_list.begin(), authors_list.end(),std::back_inserter(authors_list_case),
    [](const Author & author) -> detail::AuthorInfo {
        return detail::AuthorInfo{author.GetId().ToString(), author.GetName()};
    });
    return authors_list_case;
}

UseCases::books_list_t UseCasesImpl::GetBooks() {
    CreateNextUnitWork();

    auto books_list = last_unit_of_work_->Books().GetList();
    books_list_t books_list_case;
    std::transform(books_list.begin(), books_list.end(),std::back_inserter(books_list_case),
    [](const Book & book) -> detail::BookInfo {
        return detail::BookInfo{book.GetTitle(),book.GetYear(), book.GetAuthorName(), book.GetId().ToString()};
    });
    return books_list_case;
}

UseCases::books_list_t UseCasesImpl::GetBooksAuthors(const std::string & author_id) {
    CreateNextUnitWork();

    auto books_list = last_unit_of_work_->Books().GetBookByAuthorId(AuthorId::FromString(author_id));
    books_list_t books_list_case;
    std::transform(books_list.begin(), books_list.end(),std::back_inserter(books_list_case),
    [](const Book & book) -> detail::BookInfo {
        return detail::BookInfo{book.GetTitle(),book.GetYear()};
    });
    return books_list_case;
}

std::optional<detail::AuthorInfo> UseCasesImpl::FindAuthorByName(const std::string& name) {
    CreateNextUnitWork();

    auto author = last_unit_of_work_->Authors().FindAuthorByName(name);
    if(!author)
        return std::nullopt;
    return detail::AuthorInfo{author->GetId().ToString(), author->GetName()};
}

std::optional<detail::BookInfo> UseCasesImpl::FindBookByTitle(const std::string& title) {
    CreateNextUnitWork();

    auto book = last_unit_of_work_->Books().GetBookByTitle(title);
    if(!book)
        return std::nullopt;
    return detail::BookInfo{book->GetTitle(),book->GetYear(), book->GetAuthorName(), book->GetId().ToString()};   
}

UseCases::tag_list_t UseCasesImpl::GetTagsByBookId(const std::string& author_id) {
    CreateNextUnitWork();

    auto tags_list = last_unit_of_work_->Tags().GetTagsByBookId(BookId::FromString(author_id));
    tag_list_t tags_list_case;
    std::transform(tags_list.begin(), tags_list.end(),std::back_inserter(tags_list_case),
    [](const Tag & tag) -> std::string {
        return tag.GetTag();
    });
    return tags_list_case;
}

void UseCasesImpl::CreateNextUnitWork() {
    if(!last_unit_of_work_)
    last_unit_of_work_ = unit_factory_.CreateUnitOfWork();
}
}  // namespace app
