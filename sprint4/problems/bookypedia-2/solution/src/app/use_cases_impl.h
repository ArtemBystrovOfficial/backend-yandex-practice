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
        last_unit_of_work_ = unit_factory_.CreateUnitOfWork();
    }

    void Commit() override {
        last_unit_of_work_->Commit();
        last_unit_of_work_.reset();
        last_unit_of_work_ = unit_factory_.CreateUnitOfWork();
    }

    void Rollback() override {
        last_unit_of_work_.reset();
        last_unit_of_work_ = unit_factory_.CreateUnitOfWork();
    }

    void EditBook(const std::string & book_id, const std::string & title, int publication_year, const std::vector<std::string> & tags) override;
    void EditAuthorName(const std::string & author_id, const std::string & author_new_name) override;
    void DeleteAuthorAndDependenciesByName(const std::string & author_name) override;
    void DeleteAuthorAndDependencies(const std::string & author_id) override;
    void DeleteBookAndDependencies (std::string & book_id) override;
    std::string AddAuthor(const std::string& name) override;
    std::string AddBook(int year, const std::string & author_id, const std::string& title) override;
    void AddTags(const std::string & book_id,const std::vector<std::string> & tags) override;
    void ClearTags(const std::string & book_id);

    authors_list_t GetAuthors() override;
    books_list_t GetBooks() override;
    books_list_t GetBooksAuthors(const std::string & author_id) override;
    std::optional<detail::AuthorInfo> FindAuthorByName(const std::string & name) override;
    books_list_t FindBooksByTitle(const std::string & title) override;
    tag_list_t GetTagsByBookId(const std::string &) override;

private:
    void CascadeRemoveBooksAndTags(const domain::AuthorId & author_id);
    void CascadeRemoveTags(const domain::BookId & book_id);

    std::shared_ptr<UnitOfWork> last_unit_of_work_;
    UnitOfWorkFactory & unit_factory_;
};

}  // namespace app
