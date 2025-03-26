package parser

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

type TestCase struct {
	tokens   []Token
	expected string
}

func TestParseExpressions(t *testing.T) {
	tests := []TestCase{
		{
			tokens: []Token{{
				Type:   TokenMinus,
				Lexeme: "-",
			}, {
				Type:   TokenName,
				Lexeme: "a",
			}},
			expected: "-(a)",
		},
		{
			tokens: []Token{{
				Type:   TokenPlus,
				Lexeme: "+",
			}, {
				Type:   TokenName,
				Lexeme: "b",
			}},
			expected: "+(b)",
		},
		{
			tokens: []Token{{
				Type:   TokenBang,
				Lexeme: "!",
			}, {
				Type:   TokenName,
				Lexeme: "c",
			}},
			expected: "!(c)",
		},
		{
			tokens: []Token{{
				Type:   TokenTilde,
				Lexeme: "~",
			}, {
				Type:   TokenName,
				Lexeme: "d",
			}},
			expected: "~(d)",
		},
		{
			tokens: []Token{{
				Type:   TokenMinus,
				Lexeme: "-",
			}, {
				Type:   TokenMinus,
				Lexeme: "-",
			}, {
				Type:   TokenName,
				Lexeme: "e",
			}},
			expected: "-(-(e))",
		},
		{
			tokens: []Token{{
				Type:   TokenName,
				Lexeme: "a",
			}, {
				Type:   TokenPlus,
				Lexeme: "+",
			}, {
				Type:   TokenName,
				Lexeme: "bcd",
			}},
			expected: "(a)+(bcd)",
		},
		{
			tokens: []Token{{
				Type:   TokenName,
				Lexeme: "a",
			}, {
				Type:   TokenMinus,
				Lexeme: "-",
			}, {
				Type:   TokenName,
				Lexeme: "b",
			},

				{
					Type:   TokenMinus,
					Lexeme: "-",
				},
				{
					Type:   TokenName,
					Lexeme: "c",
				},
			},
			expected: "((a)-(b))-(c)",
		},
		{
			tokens: []Token{{
				Type:   TokenName,
				Lexeme: "a",
			}, {
				Type:   TokenMinus,
				Lexeme: "-",
			}, {
				Type:   TokenName,
				Lexeme: "b",
			},

				{
					Type:   TokenPlus,
					Lexeme: "+",
				},
				{
					Type:   TokenName,
					Lexeme: "c",
				},
			},
			expected: "((a)-(b))+(c)",
		},
		{
			tokens: []Token{{
				Type:   TokenName,
				Lexeme: "a",
			}, {
				Type:   TokenMinus,
				Lexeme: "-",
			}, {
				Type:   TokenName,
				Lexeme: "b",
			},

				{
					Type:   TokenStar,
					Lexeme: "*",
				},
				{
					Type:   TokenName,
					Lexeme: "c",
				},
			},
			expected: "(a)-((b)*(c))",
		},
		{
			tokens: []Token{{
				Type:   TokenName,
				Lexeme: "a",
			}, {
				Type:   TokenStar,
				Lexeme: "*",
			}, {
				Type:   TokenMinus,
				Lexeme: "-",
			},

				{
					Type:   TokenName,
					Lexeme: "b",
				},
			},
			expected: "(a)*(-(b))",
		},
		{
			tokens: []Token{{
				Type:   TokenName,
				Lexeme: "a",
			}, {
				Type:   TokenLeftParen,
				Lexeme: "(",
			}, {
				Type:   TokenName,
				Lexeme: "b",
			}, {
				Type:   TokenComma,
				Lexeme: ",",
			}, {
				Type:   TokenName,
				Lexeme: "c",
			}, {
				Type:   TokenPlus,
				Lexeme: "+",
			}, {
				Type:   TokenName,
				Lexeme: "d",
			}, {
				Type:   TokenRightParen,
				Lexeme: ")",
			}},
			expected: "(a)(b,(c)+(d))",
		},
		{
			tokens: []Token{{
				Type:   TokenName,
				Lexeme: "a",
			}, {
				Type:   TokenLeftParen,
				Lexeme: "(",
			}, {
				Type:   TokenRightParen,
				Lexeme: ")",
			}},
			expected: "(a)()",
		},
		{
			tokens: []Token{{
				Type:   TokenName,
				Lexeme: "a",
			}, {
				Type:   TokenLeftParen,
				Lexeme: "(",
			}, {
				Type:   TokenRightParen,
				Lexeme: ")",
			},
				{
					Type:   TokenLeftParen,
					Lexeme: "(",
				}, {
					Type:   TokenRightParen,
					Lexeme: ")",
				},
			},
			expected: "((a)())()",
		},
		{
			tokens: []Token{
				{
					Type:   TokenLeftParen,
					Lexeme: "(",
				}, {
					Type:   TokenName,
					Lexeme: "a",
				}, {
					Type:   TokenRightParen,
					Lexeme: ")",
				},
			},
			expected: "a",
		},
	}

	for _, test := range tests {
		expr := NewParser(test.tokens).ParseExpression()
		assert.Equal(t, test.expected, expr.ToString())
	}

}
