#pragma once
#include <sstream>
#include <vector>
#include <memory>
#include <deque>
#include <set>
#include <assert.h>
#include "../util.h"

namespace mathvm {
    namespace IR {

        typedef uint64_t VarId;

#define IR_COMMON_FUNCTIONS(ir) \
virtual IrElement* visit(IrVisitor<> *const v) const { return v->visit(this); }\
virtual void visit(IrVisitor<void> *const v) const { v->visit(this); }\
virtual bool is##ir() const { return true; } ;\
virtual ir const* as##ir() const { return this; } ;\
virtual IrType getType() const { return IT_##ir; }

#define FOR_IR(DO) \
        DO(BinOp)\
        DO(UnOp)\
        DO(Variable)\
        DO(Return)\
        DO(Phi)\
        DO(Int)\
        DO(Double)\
        DO(Ptr)\
        DO(Block)\
        DO(Assignment)\
        DO(Call)\
        DO(Print)\
        DO(Function)\
        DO(JumpAlways)\
        DO(JumpCond)\
        DO(WriteRef)\
        DO(ReadRef)

#define DECLARE_IR(ir) struct ir;
        FOR_IR(DECLARE_IR)
#undef DECLARE_IR
        struct IrElement;
        template<typename T = IrElement *>
        struct IrVisitor;
        struct Expression;
        struct Atom;

        class IrElement {
        public:
            enum IrType {

#define DECLARE_IR_TYPE(ir) IT_##ir,
                FOR_IR(DECLARE_IR_TYPE)
                IT_INVALID
#undef DECLARE_IR_TYPE
            };

            virtual IrType getType() const = 0;

            virtual void visit(IrVisitor<void> *const visitor) const {
            };

            virtual IrElement *visit(IrVisitor<> *const visitor) const = 0;

            virtual bool isAtom() const {
                return false;
            }

            virtual bool isLiteral() const {
                return false;
            }

            virtual bool isExpression() const {
                return false;
            }

            virtual Atom const *asAtom() const {
                return NULL;
            }

            virtual Expression const *asExpression() const {
                return NULL;
            }

#define HELPER(ir) virtual bool is##ir() const { return false; } ; virtual ir const* as##ir() const { return NULL; } ;

            FOR_IR(HELPER)
#undef HELPER

            virtual ~IrElement() {
            }
        };

        template<typename T>
        struct IrVisitor {
#define VISITORABSTR(ir) virtual T visit(const ir *const expr) = 0;

            FOR_IR(VISITORABSTR)

#undef VISITORABSTR


#define VISITOR_IRELEMENT(ir) virtual IrElement* visit(const ir * const expr) ;
#define VISITOR_VOID(ir) virtual void visit(const ir * const expr) ;

            virtual ~IrVisitor() {
            }
        };


        struct Expression : IrElement {
            virtual bool isExpression() const {
                return true;
            }

            virtual Expression const *asExpression() const {
                return this;
            }

            virtual ~Expression() {
            }
        };

        struct Atom : Expression {
            virtual bool isAtom() const {
                return true;
            }

            virtual const Atom *asAtom() const {
                return this;
            }

            virtual ~Atom() {
            }
        };

        enum VarType {
            VT_Undefined,
            VT_Unit,
            VT_Int,
            VT_Double,
            VT_Ptr,
            VT_Error
        };

        struct Variable : Atom {
            Variable(VarId id) : id(id) {
            }

            const VarId id;

            IR_COMMON_FUNCTIONS(Variable)
        };

        struct Statement : IrElement {
            mutable size_t num = 0;

            virtual ~Statement() {
            }
        };

        struct Jump : Statement {
            virtual ~Jump() {
            }
        };


        struct JumpAlways : Jump {
            Block *const destination;

            JumpAlways(Block *dest) : destination(dest) {
            }

            IR_COMMON_FUNCTIONS(JumpAlways)

            virtual ~JumpAlways() {
            }
        };

        struct JumpCond : Jump {
            JumpCond(Block *const yes, Block *const no, Atom const *const condition)
                    : yes(yes), no(no), condition(condition) {
            }

            Block *const yes;
            Block *const no;
            const Atom *const condition;

            JumpCond *replaceYes(Block *const repl) const;

            JumpCond *replaceNo(Block *const repl) const;

            virtual ~JumpCond() {
                delete condition;
            }

            IR_COMMON_FUNCTIONS(JumpCond)
        };

        struct Assignment : Statement {
            const Variable *const var;
            const Expression *const value;

            Assignment(Variable const *const var, Expression const *const expr) : var(var), value(expr) {
            }

            Assignment(VarId id, Expression const *const expr) : var(new Variable(id)), value(expr) {
            }

            IR_COMMON_FUNCTIONS(Assignment)

            virtual ~Assignment() {
                delete var;
                delete value;
            }
        };

        struct Return : Statement {
            Return(const Atom *const atom) : atom(atom) {
            }

            const Atom *const atom;

            IR_COMMON_FUNCTIONS(Return)

            virtual ~Return() {
                delete atom;
            }
        };

        struct Phi : Statement {
            const Variable *const var;

            Phi(VarId id) : var(new Variable(id)) {
            }

            Phi(Variable const *id) : var(id) {
            }

            std::set<const Variable *> vars;

            IR_COMMON_FUNCTIONS(Phi)


            virtual ~Phi() {
                delete var;
                for (auto v : vars) delete v;
            }
        };

        struct Call : Expression {
            Call(uint16_t id, std::vector<Atom const *> const &args, std::vector<VarId> const &refArgs
            )
                    : funId(id), params(args), refParams(refArgs) {
            }

            const uint16_t funId;
            std::vector<Atom const *> params;
            std::vector<VarId> refParams;

            IR_COMMON_FUNCTIONS(Call)

            virtual ~Call() {
                for (auto p : params) delete p;
            }
        };

        struct BinOp : Expression {

            const Atom *const left;
            const Atom *const right;
#define FOR_IR_BINOP(DO) \
DO(ADD, "+")\
DO(FADD, ".+.")\
DO(SUB, "-")\
DO(FSUB, ".-.")\
DO(MUL, "*")\
DO(FMUL, ".*.")\
DO(DIV, "/")\
DO(FDIV, "./.")\
DO(MOD, "%")\
DO(LT, "<")\
DO(FLT, ".<.")\
DO(LE, "<=")\
DO(FLE, ".<=.")\
DO(EQ, "==")\
DO(NEQ, "!=")\
DO(OR, "|")\
DO(AND, "&")\
DO(LOR, "||")\
DO(LAND, "&&")\
DO(XOR, "^")

#define BINOP_ENUM(name, _) BO_##name,
            enum Type {
                FOR_IR_BINOP(BINOP_ENUM)
                BO_INVALID
            };
            const Type type;

            BinOp(Atom const *left, Atom const *right, Type type) : left(left), right(right), type(type) {
            }

            IR_COMMON_FUNCTIONS(BinOp)

            virtual ~BinOp() {
                delete left;
                delete right;
            }
        };

        struct UnOp : Expression {
            const Atom *const operand;
#define FOR_IR_UNOP(DO)\
DO(CAST_I2D, "<i2d>")\
DO(CAST_D2I,"<d2i>")\
DO(NEG, "-")\
DO(FNEG, ".-.")\
DO(NOT, "!")
#define UNOP_NAME(o, _) UO_##o,

            enum Type {
                FOR_IR_UNOP(UNOP_NAME)
                UO_INVALID
            };
#undef UNOP_NAME

            const Type type;

            UnOp(const Atom *const operand, Type type) : operand(operand), type(type) {
            }

            IR_COMMON_FUNCTIONS(UnOp)


            virtual ~UnOp() {
                delete operand;
            }
        };

        struct Int : Atom {
            const int64_t value;

            virtual bool isLiteral() const {
                return true;
            }

            Int(int64_t value) : value(value) {
            }

            IR_COMMON_FUNCTIONS(Int)
        };

        struct Double : Atom {
            const double value;

            virtual bool isLiteral() const {
                return true;
            }

            Double(double value) : value(value) {
            }

            IR_COMMON_FUNCTIONS(Double)
        };

        struct Ptr : Atom {
            const uint64_t value;
            const bool isPooledString;

            virtual bool isLiteral() const {
                return true;
            }

            Ptr(VarId value, bool isPooledString) : value(value), isPooledString(isPooledString) {
            }

            IR_COMMON_FUNCTIONS(Ptr)
        };


        struct Block : IrElement {
        private:
            Jump *_transition;
        public:
            const std::string name;

            Jump *getTransition() const {
                return _transition;
            }

            void setTransition(Jump *jmp) {
                _transition = jmp;
                if (!jmp) return;
                if (jmp->isJumpAlways())
                    jmp->asJumpAlways()->destination->addPredecessor(this);
                else {
                    jmp->asJumpCond()->yes->addPredecessor(this);
                    jmp->asJumpCond()->no->addPredecessor(this);
                }
            }

            std::set<const Block *> predecessors;

            Block(std::string const &name) : _transition(NULL), name(name) {
            }

            bool isEmpty() const {
                return (contents.size() == 0) && (!isLastBlock()) && getTransition()->isJumpAlways();
            }

            bool isEntry() const {
                return predecessors.size() == 0;
            }

            void link(JumpCond *cond) {
                if (_transition) delete _transition;
                _transition = cond;
                cond->yes->addPredecessor(this);
                cond->no->addPredecessor(this);
            }

            void link(Block *next) {
                if (_transition) delete _transition;
                _transition = new JumpAlways(next);
                next->addPredecessor(this);
            }

            std::deque<Statement const *> contents;


            virtual ~Block() {
                for (auto s : contents)
                    delete s;
                delete _transition;
            }

            void addPredecessor(Block const *block) {
                predecessors.insert(block);
            }

            void removePredecessor(Block const *block) {
                predecessors.erase(block);
            }

            bool isLastBlock() const {
                return getTransition() == NULL;
            }

            bool relink(Block * oldChild, Block* newChild);
            IR_COMMON_FUNCTIONS(Block)
        };


        class Function : public IrElement {

        public:
            Function(uint16_t id, VarType returnType, std::string const& name)
                    : name(name), id(id), entry(new Block(mathvm::toString(id))), returnType(returnType), nativeAddress(NULL)  {
            }
            Function(uint16_t id, void const* address, VarType returnType, std::string const& name)
                    : name(name),  id(id), entry(new Block("NATIVE_STUB")), returnType(returnType), nativeAddress(address) {
            }

            Function(uint16_t id, VarType returnType, Block *startBlock, std::string const& name)
                    :  name(name), id(id), entry(startBlock), returnType(returnType), nativeAddress(NULL)  {
            }

            const std::string name;
            const uint16_t id;
            Block *entry;
            std::vector<VarId> parametersIds;
            std::vector<VarId> refParameterIds;
            std::vector<VarId> memoryCells; //each id is the pointer id. Write to it == write to this memory cell.
            // read from it == read from cell
            // pass it == pass the cell's address

            VarType returnType;

            const void* const nativeAddress;
            bool isNative() const { return nativeAddress != NULL; }

            IR_COMMON_FUNCTIONS(Function)


            VarId argument(size_t idx) const {
                if (idx < parametersIds.size()) return parametersIds[idx];
                idx -= parametersIds.size();
                if (idx < refParameterIds.size()) return refParameterIds[idx];
                std::stringstream msg;
                msg << "Argument index is out of range: total args: " << arguments() << " index " << idx ;
                throw new std::out_of_range(msg.str());
            }
            size_t arguments() const { return parametersIds.size() + refParameterIds.size(); }

            virtual ~Function();
        };


        struct Print : Statement {
            Print(Atom const *const atom) : atom(atom) {
            }

            const Atom *const atom;

            IR_COMMON_FUNCTIONS(Print)

            virtual ~Print() {
                delete atom;
            }
        };

        struct WriteRef : Statement {
            WriteRef(Atom const *const atom, VarId const where) : atom(atom), refId(where) {
            }

            const Atom *const atom;
            const VarId refId;

            IR_COMMON_FUNCTIONS(WriteRef)

            virtual ~WriteRef() {
                delete atom;
            }
        };

        struct ReadRef : Atom {
            const VarId refId;

            IR_COMMON_FUNCTIONS(ReadRef)

            ReadRef(VarId const refId) : refId(refId) {
            }

        };

        struct SimpleIr {

            struct VarMeta {
                const VarId id;
                const bool isSourceVar;
                bool isReference;
                const VarId originId;
                const Function *pointsTo;
                const uint32_t offset;

                VarType type;

                VarMeta(VarId id, VarId from, VarType type)
                        : id(id),
                          isSourceVar(true),
                          isReference(false),
                          originId(from),
                          pointsTo(NULL),
                          offset(0),
                          type(type) {
                }
                VarMeta(VarId id,  VarType type)
                        : id(id),
                          isSourceVar(true),
                          isReference(false),
                          originId(0),
                          pointsTo(NULL),
                          offset(0),
                          type(type) {
                }

                VarMeta(VarMeta const &meta) : id(meta.id),
                                               isSourceVar(meta.isSourceVar),
                                               isReference(meta.isReference),
                                               originId(meta.originId),
                                               pointsTo(meta.pointsTo),
                                               offset(meta.offset),
                                               type(meta.type) {
                }

                VarMeta(VarId id)
                        : id(id),
                          isSourceVar(false),
                          isReference(false),
                          originId(0),
                          pointsTo(NULL),
                          offset(0),
                          type(VT_Undefined) {
                }

                VarMeta(VarId id, VarType type, Function const *pointsTo, uint32_t offset)
                        : id(id),
                          isSourceVar(false),
                          isReference(true),
                          originId(0),
                          pointsTo(pointsTo),
                          offset(offset),
                          type(type) {
                }
            };

            typedef std::vector<std::string> StringPool;

            StringPool pool;
            std::vector<Function *> functions;

            void addFunction(Function *rec) {
                functions.push_back(rec);
            }

            std::vector<VarMeta> varMeta;

            virtual ~SimpleIr() {
                for (auto f : functions)
                    delete f;
            }
        };
    }
}