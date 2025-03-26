package parser

type Parser struct {
	prefix map[TokenType]Parselet
	infix  map[TokenType]InfixParselet
	tokens []Token
	curr   int
}

func NewParser(tokens []Token) *Parser {
	p := &Parser{
		prefix: make(map[TokenType]Parselet),
		infix:  make(map[TokenType]InfixParselet),
		tokens: tokens,
	}
	p.register(TokenName, NameParselet)
	p.register(TokenLeftParen, GroupParselet)
	p.registerPrefix(TokenPlus)
	p.registerPrefix(TokenBang)
	p.registerPrefix(TokenMinus)
	p.registerPrefix(TokenTilde)
	///
	p.registerBinary(TokenPlus)
	p.registerBinary(TokenMinus)
	p.registerBinary(TokenSlash)
	p.registerBinary(TokenStar)

	p.registerInfix(TokenLeftParen, CallParselet)
	return p
}

func (p *Parser) register(tok TokenType, fn Parselet) {
	p.prefix[tok] = fn
}

func (p *Parser) registerPrefix(tok TokenType) {
	p.register(tok, PrefixParselet)
}

func (p *Parser) registerInfix(tok TokenType, parselet InfixParselet) {
	p.infix[tok] = parselet
}

func (p *Parser) registerBinary(tok TokenType) {
	p.registerInfix(tok, func(p *Parser, left Expression, tok Token) Expression {
		precedence := int(precedences[tok.Type])
		right := p.parsePrecedence(precedence + 1)
		return NewBinaryExpression(tok, left, right)
	})
}

func (p *Parser) isAtEnd() bool {
	return p.curr == len(p.tokens)
}

func (p *Parser) consume() Token {
	if p.isAtEnd() {
		return Token{Type: TokenEof}
	}
	p.curr += 1
	return p.tokens[p.curr-1]
}

func (p *Parser) peek() Token {
	if p.isAtEnd() {
		return Token{Type: TokenEof}
	}
	return p.tokens[p.curr]
}

// get the precedence of the next token
func (p *Parser) getPrecedence() int {
	return int(precedences[p.peek().Type])
}

func (p *Parser) match(tok TokenType) bool {
	if p.peek().Type == tok {
		p.consume()
		return true
	}
	return false
}

func (p *Parser) parsePrecedence(precedence int) Expression {
	token := p.consume()
	parselet := p.prefix[token.Type]
	if parselet == nil {
		panic("Invalid token")
	}
	expr := parselet(p, token)

	for {
		token = p.peek()
		infixParselet := p.infix[token.Type]
		if infixParselet == nil {
			break
		}
		nextPrecedence := p.getPrecedence()
		// the next token has lower precedence
		if nextPrecedence < precedence {
			break
		}
		p.consume()
		expr = infixParselet(p, expr, token)
	}
	return expr
}

func (p *Parser) expect(tok TokenType) {
	if !p.match(tok) {
		panic("Expected token")
	}
}

func (p *Parser) ParseExpression() Expression {
	return p.parsePrecedence(0)
}
