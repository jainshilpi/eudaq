#ifndef HEXAPOS_H
#define HEXAPOS_H

// system includes
#include <vector>

// Hexagonal position list
class HexGrid {
    /*
     _______
    /   |   \
   /    |    \
  /_____|_____\
  \     |     /
   \    |    /
    \   |   /
     -------

    */

protected:

    // Hexagon parameters [mm]
    double m_largeD = 135.;
    double m_smallD = 125.;

    double m_largeR = m_largeD / 2.;
    double m_smallR = m_smallD / 2.;

    // side parameters
    // y = ax + b for the top-right edge
    // x = (y-b)/a
    double m_b =  2. * m_smallR;
    double m_a = -2. * m_smallR/m_largeR;

    // center positions [mm]
    double m_centerX = 0.;
    double m_centerY = 0.;

    // step sizes [mm]
    double m_stepSizeX = 50.;
    double m_stepSizeY = 25.;

    // border offset [mm]
    double m_borderOffsetX = 0;
    double m_borderOffsetY = 0;

    // start positions [mm]
    double m_startY = m_smallR - m_borderOffsetY;
    double m_startX = - (m_startY - m_b) / m_a + m_borderOffsetX;

    // Position list/vector
    std::vector<double> m_positionsX;
    std::vector<double> m_positionsY;

    std::vector< std::vector<double> > m_positions;

    unsigned int m_npoints = 0;

public:

    HexGrid();
    HexGrid(double centerX, double centerY);

    ~HexGrid();

    int BuildGrid();

    // functions to set parameters
    void setCenterX(double posX) { m_centerX = posX; } ;
    void setCenterY(double posY) { m_centerY = posY; } ;
    void setStepX(double step) { m_stepSizeX = step; };
    void setStepY(double step) { m_stepSizeY = step; };
 
    void setStartX(double pos) { m_startX = pos; };
    void setStartY(double pos) { m_startY = pos; };


    // update Hexagon parameters
    void setLargeD(double largeD) {
	m_largeD = largeD;
	m_largeR = m_largeD / 2.;
	m_a = -2. * m_smallR/m_largeR;

	m_startX = - (m_startY - m_b) / m_a + m_borderOffsetX;
    } ;

    void setSmallD(double smallD) {
	m_smallD = smallD;
	m_smallR = m_smallD / 2.;
	m_b =  2. * m_smallR;

	m_startY = m_smallR - m_borderOffsetY;
    } ;


    // functions to get parameters
    double getPosX(unsigned posID) { return (posID < m_npoints)? m_positionsX.at(posID):-1; };
    double getPosY(unsigned posID) { return (posID < m_npoints)? m_positionsY.at(posID):-1; };
    int getNpos() {return m_npoints;};
};

#endif
