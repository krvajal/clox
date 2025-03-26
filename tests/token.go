package parser

type TokenType int

const (
	TokenName TokenType = iota
	TokenPlus
	TokenMinus
	TokenTilde
	TokenBang
	TokenEof
	TokenSlash
	TokenStar
	TokenLeftParen
	TokenRightParen
	TokenComma
)

type Token struct {
	Type   TokenType
	Lexeme string
}
