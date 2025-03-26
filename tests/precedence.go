package parser

type Precedence int

const (
	PrecedenceNone Precedence = iota
	PrecedenceAssignment
	PrecedenceSum
	PrecedenceProd
	PrecedenceCall
	PrecedencePrefix
	PredencePosfix
)

var precedences = map[TokenType]Precedence{
	TokenName:  PrecedenceNone,
	TokenBang:  PrecedencePrefix,
	TokenPlus:  PrecedenceSum,
	TokenMinus: PrecedenceSum,
	// prod
	TokenSlash:     PrecedenceProd,
	TokenStar:      PrecedenceProd,
	TokenLeftParen: PrecedenceCall,
}
