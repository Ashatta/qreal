#pragma once

#include "qrtext/core/parser/operators/parserInterface.h"
#include "qrtext/core/parser/parserRef.h"
#include "qrtext/core/parser/temporaryNodes/temporaryErrorNode.h"

namespace qrtext {
namespace core {

/// Elementary parser for parsing alternatives in form of A ::= B | C. Alternatives shall have distinct FIRST sets.
/// Returns either first or second parser result, or nullptr if both parsers failed.
template<typename TokenType>
class AlternativeParser : public ParserInterface<TokenType>
{
public:
	/// Constructor. Takes parsers for two alternatives.
	AlternativeParser(const ParserRef<TokenType> &parser1, const ParserRef<TokenType> &parser2)
		: mParser1(parser1), mParser2(parser2)
	{
	}

	QSharedPointer<ast::Node> parse(TokenStream<TokenType> &tokenStream
			, ParserContext<TokenType> &parserContext) const override
	{
		if (tokenStream.isEnd()) {
			parserContext.reportError(QObject::tr("Unexpected end of input"));
			return wrap(new TemporaryErrorNode());
		}

		if (!(mParser1->first().intersect(mParser2->first())).isEmpty()) {
			parserContext.reportInternalError(
					QObject::tr("Parser can not decide which alternative to use on ") + tokenStream.next().lexeme());
		}

		if (mParser1->first().contains(tokenStream.next().token())) {
			return mParser1->parse(tokenStream, parserContext);
		}

		if (mParser2->first().contains(tokenStream.next().token())) {
			return mParser2->parse(tokenStream, parserContext);
		}

		parserContext.reportError(QObject::tr("Unexpected token"));
		return wrap(new TemporaryErrorNode());
	}

	QSet<TokenType> first() const override
	{
		return mParser1->first() + mParser2->first();
	}

private:
	ParserRef<TokenType> mParser1;
	ParserRef<TokenType> mParser2;
};

}
}
