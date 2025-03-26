package parser

type Parselet func(parser *Parser, token Token) Expression

func PrefixParselet(parser *Parser, token Token) Expression {
	precedence := int(precedences[token.Type])
	// left associative
	return NewPrefixExpression(token, parser.parsePrecedence(precedence+1))
}

func NameParselet(parser *Parser, token Token) Expression {
	return NewNameExpression(token)
}

type InfixParselet func(parser *Parser, left Expression, token Token) Expression
