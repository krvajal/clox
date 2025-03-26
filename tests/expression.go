package parser

type Expression interface {
	ToString() string
}

type NameExpression struct {
	token Token
}

// ToString implements Expression.
func (expr NameExpression) ToString() string {
	return expr.token.Lexeme

}

func NewNameExpression(token Token) NameExpression {
	return NameExpression{
		token: token,
	}
}

type PrefixExpression struct {
	prefix Token
	expr   Expression
}

func NewPrefixExpression(prefix Token, expr Expression) PrefixExpression {
	return PrefixExpression{
		prefix: prefix,
		expr:   expr,
	}
}

// ToString implements Expression.
func (expr PrefixExpression) ToString() string {
	return expr.prefix.Lexeme + "(" + expr.expr.ToString() + ")"
}

type BinaryExpression struct {
	left     Expression
	operator Token
	right    Expression
}

func NewBinaryExpression(operator Token, left Expression, right Expression) BinaryExpression {
	return BinaryExpression{
		left:     left,
		operator: operator,
		right:    right,
	}
}

// ToString implements Expression.
func (expr BinaryExpression) ToString() string {
	return "(" + expr.left.ToString() + ")" + expr.operator.Lexeme + "(" + expr.right.ToString() + ")"
}
