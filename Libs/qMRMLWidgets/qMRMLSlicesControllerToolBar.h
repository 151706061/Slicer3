#ifndef __qMRMLSlicesControllerToolBar_h
#define __qMRMLSlicesControllerToolBar_h

// Qt includes
#include <QSignalMapper>
#include <QToolBar>

// CTK includes
#include <ctkPimpl.h>
#include "qMRMLWidgetsExport.h"

class qMRMLSlicesControllerToolBarPrivate;
class vtkMRMLScene;

class QMRML_WIDGETS_EXPORT qMRMLSlicesControllerToolBar : public QToolBar
{
  Q_OBJECT
public:
  /// Superclass typedef
  typedef QToolBar Superclass;

  /// Constructors
  explicit qMRMLSlicesControllerToolBar(QWidget* parent = 0);
  virtual ~qMRMLSlicesControllerToolBar(){}

  vtkMRMLScene* mrmlScene()const;

public slots:
  void setMRMLScene(vtkMRMLScene* scene);
  void setFiducialPointsVisible(bool visible);
  void setFiducialLabelsVisible(bool visible);
  void setForegroundGridVisible(bool visible);
  void setBackgroundGridVisible(bool visible);
  void setLabelGridVisible(bool visible);
  void fitToWindow();
  void setNavigatorEnabled(bool enable);
  void setSliceIntersectionVisible(bool visible);
  void toggleBackgroundForeground();
  void showBackground();
  void showForeground();
  void setForegroundOpacity(double value);
  void setAnnotationMode(int mode);
  void setCompositing(int mode);
  void setCrosshairMode(int mode);
  void setCrosshairThickness(int mode);
  void setAnnotationSpace(int mode);

private:
  CTK_DECLARE_PRIVATE(qMRMLSlicesControllerToolBar);
};

class qMRMLActionSignalMapper: public QSignalMapper
{
  Q_OBJECT
public:
  qMRMLActionSignalMapper(QObject* parent=0);
public slots:
  void map(QAction* sender);
};

#endif
