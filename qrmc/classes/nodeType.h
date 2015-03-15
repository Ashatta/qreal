#pragma once

#include "graphicType.h"

#include <QList>

namespace qrmc {
	class MetaCompiler;

	class NodeType : public GraphicType
	{
	public:
		NodeType(Diagram *diagram, qrRepo::LogicalRepoApi *api, const qReal::Id id);
		virtual Type* clone() const;
		virtual ~NodeType();

		virtual bool init(const QString &context);

		virtual void print();

		virtual QString generateIsNodeOrEdge(const QString &lineTemplate) const;
		virtual QString generateNodeClass(const QString &classTemplate);
		virtual QString generateEdgeClass(const QString &classTemplate) const;
		virtual QString generateResourceLine(const QString &resourceTemplate) const;

	private:
		void generateContainerStuff(QString &classTemplate) const;
		void generateContextMenuItems(QString &classTemplate, MetaCompiler *compiler) const;
		QString loadBoolProperty(const qReal::Id &id, const QString &property) const;
		QString loadIntProperty(const qReal::Id &id, const QString &property) const;
		QString loadIntVectorProperty(const qReal::Id &id, const QString &property) const;
	};
}
