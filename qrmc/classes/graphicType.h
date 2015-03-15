#pragma once

#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QPair>

#include "type.h"
#include "shape.h"

namespace qrmc {
	class Label;
	class Diagram;

	const int maxLineLength = 80;

	class GraphicType : public Type
	{
	public:
		GraphicType(Diagram *diagram, qrRepo::LogicalRepoApi *api, const qReal::Id &id);
		virtual ~GraphicType();
		virtual bool init(const QString &context);
		virtual bool resolve();

		virtual bool isGraphicalType() const;
		virtual QString generateProperties(const QString &lineTemplate) const;
		virtual QString generatePropertyDefaults(const QString &namesTemplate) const;
		virtual QString generatePropertyDisplayedNames(const QString &lineTemplate) const;
		virtual QString generateReferenceProperties(const QString &lineTemplate) const;
		virtual QString generateParents(const QString &lineTemplate) const;
		virtual QString generateContainers(const QString &lineTemplate) const;
		virtual QString generateConnections(const QString &lineTemplate) const;
		virtual QString generateUsages(const QString &lineTemplate) const;
		virtual QString generateEnums(const QString &lineTemplate) const;
		virtual QString generatePossibleEdges(const QString &lineTemplate) const;

		bool isResolving() const;

	protected:
		typedef QPair<QPair<QString,QString>,QPair<bool,QString> > PossibleEdge;

		struct ContainerProperties {
			ContainerProperties();

			bool isSortContainer;
			int sizeOfForestalling;
			int sizeOfChildrenForestalling;
			bool isChildrenMovable;
			bool isMinimizingToChildren;
			bool isClass;
			bool isMaximizingChildren;
		};

		QStringList mParents;
		QStringList mChildren;
		bool mIsVisible;
		int mWidth;
		int mHeight;
		QStringList mContains;
		ContainerProperties mContainerProperties;
		QStringList mConnections;
		QStringList mUsages;
		QStringList mContextMenuItems;
		QList<PossibleEdge> mPossibleEdges;
		QStringList mBonusContextMenuFields;
		Shape mShape;

		void copyFields(GraphicType *type) const;
		QString resourceName(const QString &resourceType) const;

	private:
		class ResolvingHelper {
		public:
			ResolvingHelper(bool &resolvingFlag);
			~ResolvingHelper();
		private:
			bool &mResolvingFlag;
		};

		bool addProperty(Property *property);
		bool initPossibleEdges();
		void initShape();

		bool mResolving;
	};
}
