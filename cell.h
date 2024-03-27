#pragma once

#include "common.h"
#include "formula.h"

#include <set>
#include <optional>
#include <string_view>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);//
    ~Cell();

    void Set(std::string text); //
    void Clear(); // 

    Value GetValue() const override; //
    std::string GetText() const override; //
        
    std::vector<Position> GetReferencedCells() const override; //

private:
    class Impl {
    public:
        virtual ~Impl() = default;
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const;
        virtual void InvalidateCache() = 0;

    };        
    
    class EmptyImpl : public Impl {
    public:
        Value GetValue() const override {
            return "";
        }
        std::string GetText() const override {
            return "";
        }

        std::vector<Position> GetReferencedCells() const override { 
            return {};
        }
        
        void InvalidateCache() override {
        }
    };

    class TextImpl : public Impl {
    public:
        explicit TextImpl(std::string_view text);
        Value GetValue() const override;
        std::string GetText() const override;

        std::vector<Position> GetReferencedCells() const override { 
            return {};
        }
        
        void InvalidateCache() override {
        }

    private:
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:
        explicit FormulaImpl(std::string_view text, SheetInterface& sheet);
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
        void InvalidateCache() override {
            valid_cache_.reset();
        }
       
    private:
        std::unique_ptr<FormulaInterface> form_ptr_;
        mutable std::optional<FormulaInterface::Value> valid_cache_;
        SheetInterface& sheet_;
    };

    //std::vector<Position> FillReferenceStack(const Impl& impl) const;
    bool DoFindCircularDependency(const Impl& current_impl, std::unordered_set<const Impl*>& visited) const;
    bool FindCircularDependency(const Impl& impl) const; //
    void RefreshDependencies(); //
    void InvalidateCache(); //
    Sheet& sheet_;
    std::unique_ptr<Impl> impl_;
    std::set<Cell*> dependent_cells_;  //ячейки, которые зависят от текущей
    std::set<Cell*> referenced_cells_; //ячейки, задействованные в формуле
    
};