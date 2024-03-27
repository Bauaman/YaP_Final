#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <unordered_set>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#ARITHM!";
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) 
        : ast_(ParseFormulaAST(std::move(expression)))
    {}

    Value Evaluate(const SheetInterface& sheet) const override  {
        const std::function<double(Position)> args = [&sheet](const Position pos)->double {
            if (!pos.IsValid()) {
                throw FormulaError(FormulaError::Category::Ref);
            }
            const CellInterface* cell = sheet.GetCell(pos);
            if (!cell) {
                return 0;
            }
            if (std::holds_alternative<double>(cell->GetValue())) {
                return std::get<double>(cell->GetValue());
            }
            if (std::holds_alternative<std::string>(cell->GetValue())) {
                auto val = std::get<std::string>(cell->GetValue());
                double res = 0;
                if (!val.empty()) {
                    std::istringstream in(val);
                    if (!(in >> res) || !in.eof()) throw FormulaError(FormulaError::Category::Value);
                }
                return res;
            }
            throw FormulaError(std::get<FormulaError>(cell->GetValue()));
        };
        try {
            return ast_.Execute(args);
        } catch (const FormulaError& error) {
            return error;
        }
    }
    
    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

        class Hash {
    public:
        size_t operator()(const Position p) const {
            return std::hash<std::string>()(p.ToString());
        }
    };

    class Comp {
    public:
        bool operator()(const Position& lhs, const Position& rhs) const {
            return lhs == rhs;
        }
    };

    virtual std::vector<Position> GetReferencedCells() const override {
        std::unordered_set<Position, Hash, Comp> cells;
        for (Position cell : ast_.GetCells()) {
            if (cell.IsValid()) {
                cells.emplace(cell);
            }
        }
        std::vector<Position> res;
        for (Position item : cells) {
            res.push_back(item);
        }
        return res;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}