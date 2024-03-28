#include "sheet.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    //Убедимся, что у нас достаточно строк и столбцов для позиции
    if (pos.row >= static_cast<int>(cells_.size())) {
        cells_.resize(pos.row + 1);
    }
    if (pos.col >= static_cast<int>(cells_[pos.row].size())) {
        cells_[pos.row].resize(pos.col + 1);
    }
    if (!cells_[pos.row][pos.col]) {
        cells_[pos.row][pos.col] = std::make_unique<Cell>(*this);
    }
    cells_[pos.row][pos.col] -> Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetConcreteCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetConcreteCell(pos);
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }

    if (pos.row < static_cast<int>(cells_.size()) && pos.col < static_cast<int>(cells_[pos.row].size())) {
        if (cells_[pos.row][pos.col]) {
            cells_.at(pos.row).at(pos.col)->Clear();
        }
    }
}

Size Sheet::GetPrintableSize() const {

    if (cells_.begin() == cells_.end()) {
        return {0,0};
    }
    Size result{0, 0};
    for (int row = 0; row < static_cast<int>(cells_.size()); ++row) {
        for (int col = static_cast<int>(cells_[row].size() - 1); col >= 0; --col) {
            if (cells_[row][col]) {
                if (cells_[row][col]->GetText().empty()) {
                    continue;
                } else {
                    result.rows = std::max(result.rows, row + 1);
                    result.cols = std::max(result.cols, col + 1);
                    break;
                }
            }
        }
    }
    return result;
}

void Sheet::PrintCells(std::ostream& output, const std::function<void(const CellInterface&)>& printCell) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col) {
                output << "\t";
            }
            if (col < static_cast<int>(cells_[row].size())) {
                const CellInterface* cell = cells_[row][col].get();
                if (cell) {
                    printCell(*cell);
                }
            }
        }
        output << "\n";
    }
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    //std::cout << "GetConcrete cell (" << pos.row << "," << pos.col << ")" << std::endl; 
    if (pos.row < static_cast<int>(cells_.size()) && pos.col < static_cast<int>(cells_[pos.row].size())) {
            /*if (cells_[pos.row][pos.col].get()->GetText() == "") {
                return nullptr;
            }*/
            return cells_[pos.row][pos.col].get();
    }
    return nullptr;
}

Cell* Sheet::GetConcreteCell(Position pos) {
    return const_cast<Cell*>(
        static_cast<const Sheet&>(*this).GetConcreteCell(pos));
}

std::ostream& operator<<(std::ostream& out, const CellInterface::Value& val) {
    std::visit([&out](const auto& v) {
        out << v;
    }, val);
    return out;
}

void Sheet::PrintValues(std::ostream& output) const {
    PrintCells(output, [&output](const CellInterface& cell) {
        output << cell.GetValue();
    });
}

void Sheet::PrintTexts(std::ostream& output) const {
    PrintCells(output, [&output](const CellInterface& cell) {
        output << cell.GetText();
    });
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}