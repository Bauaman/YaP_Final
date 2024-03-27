#include "cell.h"

#include "sheet.h"

#include <stack>

Cell::Cell(Sheet& sheet) :
    sheet_(sheet),
    impl_(std::make_unique<EmptyImpl>())
    {}

Cell::~Cell() 
{}

void Cell::InvalidateCache() {
    for (Cell* dep_cell : dependent_cells_) {
        dep_cell->impl_->InvalidateCache();
        dep_cell->InvalidateCache();
    }
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

void Cell::RefreshDependencies() {
    for (Cell* ref_cell : referenced_cells_) {
        ref_cell->dependent_cells_.erase(this);
    }
    referenced_cells_.clear();
    for (const Position& pos : impl_->GetReferencedCells()) {
        Cell* ref_cell = sheet_.GetConcreteCell(pos);
        if (!ref_cell) {
            sheet_.SetCell(pos,"");
            ref_cell = sheet_.GetConcreteCell(pos);
        }
        referenced_cells_.insert(ref_cell);
        ref_cell->dependent_cells_.insert(this);
    }
}

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> temp;
    if (text.empty()) {
        temp = std::make_unique<EmptyImpl>();
    } else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
        temp = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    } else {
        temp = std::make_unique<TextImpl>(std::move(text));
    }

    if (FindCircularDependency(*temp)) {
        throw CircularDependencyException("");
    }
    impl_ = std::move(temp);

    RefreshDependencies();
    InvalidateCache();
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return form_ptr_->GetReferencedCells();
}

Cell::TextImpl::TextImpl(std::string_view text) :
    text_(std::move(text))
{
}

Cell::Value Cell::TextImpl::GetValue() const {
    if (text_[0] == ESCAPE_SIGN) {
        return text_.substr(1);
    }
    return text_;
}

std::string Cell::TextImpl::GetText() const {
    return text_;
}


Cell::FormulaImpl::FormulaImpl(std::string_view text, SheetInterface& sheet) :
    sheet_(sheet)
    {
        text = text.substr(1);
        form_ptr_ = ParseFormula(std::string(text));
    }

/*
bool Cell::FindCircularDependency(const Impl& impl) const {

    const auto referenced_cells = impl.GetReferencedCells();
    if (!referenced_cells.empty()) {
        std::unordered_set<const Cell*> ref_cells;
        for (const Position& pos : referenced_cells) {
            ref_cells.emplace(sheet_.GetConcreteCell(pos));
        }
        std::unordered_set<const Cell*> visited;
        std::stack<const Cell*> to_visit;
        to_visit.push(this);
        while (!to_visit.empty()) {
            const Cell* cur_cell = to_visit.top();
            to_visit.pop();
            visited.emplace(cur_cell);
            if (ref_cells.count(cur_cell) > 0) {
                return true;
            }
            for (const Cell* dep_cell : cur_cell->dependent_cells_) {
                if (visited.count(dep_cell) == 0) {
                    to_visit.push(dep_cell);
                }
            }
        }
    }
    return false;
}
*/

bool Cell::DoFindCircularDependency(const Impl& current_impl, std::unordered_set<const Impl*>& visited) const {
    if (visited.find(&current_impl) != visited.end()) {
        return true;
    }
    visited.insert(&current_impl);
    for (const Position& ref_cell : current_impl.GetReferencedCells()) {
        Cell* cell = sheet_.GetConcreteCell(ref_cell);
        if (cell && cell->impl_) {
            if (DoFindCircularDependency(*cell->impl_, visited)) {
                return true;
            }
        }
    }
    visited.erase(&current_impl);
    return false;
}

bool Cell::FindCircularDependency(const Impl& impl) const {
    std::unordered_set<const Impl*> visited;
    return DoFindCircularDependency(impl, visited);
}

Cell::Value Cell::FormulaImpl::GetValue() const {
    if (!valid_cache_) {
        valid_cache_ = form_ptr_->Evaluate(sheet_);
    }
    return std::visit([](auto& func){return Value(func);}, *valid_cache_);
} 

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + form_ptr_->GetExpression();
}
