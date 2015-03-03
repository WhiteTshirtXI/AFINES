/*
 *  filament.cpp
 *  
 *
 *  Created by Simon Freedman on 12/22/2014
 *  Copyright 2014 University of Chicago. All rights reserved.
 *
 */

#include "filament.h"
#include "actin.h"
#include "globals.h"
//using namespace std;
filament::filament(){

    fov[0] = 50;
    fov[1] = 50;
    nq[0] = 100;
    nq[1] = 100;
    dt = 0.001;
    temperature = 0;
    gamma = 0;
    fracture_force = 1000000;
    viscosity = 0.5;
    BC = "REFLECTIVE";

}

filament::filament(double startx, double starty, double startphi, int nrod, double fovx, double fovy, int nqx, int nqy, 
        double visc, double deltat, double temp, bool isStraight,
        double rodLength, double linkLength, double stretching_stiffness, double bending_stiffness,
        double frac_force, string bdcnd)
{
    fov[0] = fovx;
    fov[1] = fovy;
    nq[0] = nqx;
    nq[1] = nqy;
    dt = deltat;
    temperature = temp;
    gamma = 0;
    fracture_force = frac_force;
    viscosity = visc;
    BC = bdcnd;

    
    //the start of the polymer: 
    rods.push_back(new actin( startx, starty, startphi, rodLength, fov[0], fov[1], nq[0], nq[1], visc));
    lks.push_back( new Link(linkLength, stretching_stiffness, bending_stiffness, this, -1, 0) );  
    
    double  xcm, ycm, lphi, phi, variance;
    phi = startphi;
    
    if (temp != 0) variance = temp/(bending_stiffness*rodLength*rodLength);
    else variance = 1;

    for (int j = 1; j < nrod; j++) {

        // Calculate the Next rod on the actin polymer--  continues from the link
        if (!isStraight){ 

            phi += rng(-1*maxSmallAngle , maxSmallAngle);
            //phi += rng_n(0, variance);
        }
            
        lphi = (phi + rods.back()->get_angle())/2;
        xcm = rods.back()->get_end()[0] + linkLength*cos(lphi) + rodLength*0.5*cos(phi);
        ycm = rods.back()->get_end()[1] + linkLength*sin(lphi) + rodLength*0.5*sin(phi);

        // Check that this monomer is in the field of view; if not stop building the polymer
        if (       xcm > (0.5*(fov[0] - rodLength)) || xcm < (-0.5*(fov[0] - rodLength)) 
                || ycm > (0.5*(fov[1] - rodLength)) || ycm < (-0.5*(fov[1] - rodLength))      )
        {
            cout<<"DEBUG:"<<j+1<<"th segment of filament outside field of view; stopped building filament\n";
            break;
        }else{
            // Add the segment
            rods.push_back( new actin(xcm, ycm, phi, rodLength, fov[0], fov[1], nq[0], nq[1], visc) );
            lks.push_back( new Link(linkLength, stretching_stiffness, bending_stiffness, this, j-1, j) );  
        } 

    }
   
    lks.push_back( new Link(linkLength, stretching_stiffness, bending_stiffness, this, rods.size()-1, -1) );  
}

filament::filament(vector<actin *> rodvec, double linkLength, double stretching_stiffness, double bending_stiffness, 
        double deltat, double temp, double frac_force, double g, string bdcnd)
{
    
    dt = deltat;
    temperature = temp;
    fracture_force = frac_force;
    gamma = g; 
    BC = bdcnd;

    if (rodvec.size() > 0)
    {
        fov[0] = rodvec[0]->get_fov()[0];
        fov[1] = rodvec[0]->get_fov()[1];
        nq[0] = rodvec[0]->get_nq()[0];
        nq[1] = rodvec[0]->get_nq()[1];
    }
    else{
        fov[0] = 0;
        fov[1] = 0;
        nq[0] = 0;
        nq[1] = 0;
    }

    //Link em up
    for (unsigned int j = 0; j < rodvec.size(); j++) {
        
//        cout<<"\nDEBUG: adding new rod "<<j;
//      rods.push_back(new actin(*(rodvec[j])));
        actin * a = new actin(rodvec[j]->get_xcm(), rodvec[j]->get_ycm(), rodvec[j]->get_angle(), rodvec[j]->get_length(), rodvec[j]->get_fov()[0], rodvec[j]->get_fov()[1],
                    rodvec[j]->get_nq()[0], rodvec[j]->get_nq()[1], rodvec[j]->get_viscosity());
        rods.push_back(a); 
//        cout<<"\nDEBUG: adding new link "<<j;
        lks.push_back( new Link(linkLength, stretching_stiffness, bending_stiffness, this, j-1, j) );  
        //cout<<"\nDEBUG: creating rod pointer "<<rods[j];
    }

    if (rods.size() > 0){
        lks.push_back( new Link(linkLength, stretching_stiffness, bending_stiffness, this, rods.size() - 1, -1) );  
    }

    int s = rodvec.size();
    for (int i = 0; i<s; i++)
    {
        delete rodvec[i];
    }
    rodvec.clear();
    
}

filament::~filament(){
    
    int nr = rods.size(), nl = lks.size();
    for (int i = 0; i < nr; i ++)
    {    
        //cout<<"\nDEBUG: deleting pointer "<<rods[i];     
        delete rods[i];
    }
    for (int i = 0; i < nl; i ++)
        delete lks[i];
    
    rods.clear();
    lks.clear();
}

void filament::add_rod(actin * a, double linkLength, double stretching_stiffness, double bending_stiffness){
    
    rods.push_back(new actin(*a));
    
    if (lks.size() == 0)
        lks.push_back( new Link(linkLength, stretching_stiffness, bending_stiffness, this, -1,  0) );  
    else 
        lks[lks.size()-1]->set_aindex1(rods.size()-1);
    
    lks.push_back( new Link(linkLength, stretching_stiffness, bending_stiffness, this, rods.size() - 1, -1) );  

}

vector<vector<vector<int> > > filament::get_quadrants()
{
    //should return a map between rod and x, y coords of quadrant
    vector<vector<vector<int> > > quads;
    
    for (unsigned int i=0; i<rods.size(); i++) {
        
        quads.push_back(rods[i]->get_quadrants());
    }
    return quads;
}

void filament::update(double t)
{
    double vpar, vperp, vx, vy, omega, alength, xnew, ynew, phinew, a_ends[4]; 
    double xleft = -fov[0]*0.5, xright=fov[0]*0.5;
    bool recenter_filament = false;
    double yleft  = -fov[1] * 0.5;
    double yright =  fov[1] * 0.5;

    for (unsigned int i = 0; i < rods.size(); i++){
        //cout<<"\nDEBUG: rod "<<i<<" start: ( "<<rods[i]->get_start()[0]<<" , "<<rods[i]->get_start()[1]<<")";
        //cout<<"\nDEBUG: rod "<<i<<" end  : ( "<<rods[i]->get_end()[0]<<" , "<<rods[i]->get_end()[1]<<")";

    //    if(rods[i]->get_forces()[0] != rods[i]->get_forces()[0] ||rods[i]->get_forces()[1] != rods[i]->get_forces()[1] ||rods[i]->get_forces()[2] != rods[i]->get_forces()[2])
      //      cout<<"\nDEBUG: inf force on rod ["<<i<<"]";
        array<double,3> fric = rods[i]->get_frictions();
        vpar  = (rods[i]->get_forces()[0])/fric[0]  + sqrt(2*temperature/(dt*fric[0]))*rng_n(0,1);
        vperp = (rods[i]->get_forces()[1])/fric[1]  + sqrt(2*temperature/(dt*fric[1]))*rng_n(0,1);
       // cout<<"\nDEBUG: rod "<<i<<" (vpar, vperp) = ( "<<vpar<<" , "<<vperp<<" )"; 
        vx    = vpar*cos(rods[i]->get_angle()) - vperp*sin(rods[i]->get_angle());
        vy    = vpar*sin(rods[i]->get_angle()) + vperp*cos(rods[i]->get_angle());
       // cout<<"\nDEBUG: rod "<<i<<" (vx, vy) = ( "<<vx<<" , "<<vy<<" )"; 
        omega = rods[i]->get_forces()[2]/fric[2] + sqrt(2*temperature/(dt*fric[2]))*rng_n(0,1);

        alength=rods[i]->get_length();

       //cout<<"\nDEBUG: rod "<<i<<" (xold, yold) = ( "<<rods[i]->get_xcm()<<" , "<<rods[i]->get_ycm()<<" )"; 
        xnew = rods[i]->get_xcm()+dt*vx;
        ynew = rods[i]->get_ycm()+dt*vy;
       //cout<<"\nDEBUG: rod "<<i<<" (xnew, ynew) = ( "<<xnew<<" , "<<ynew<<" )"; 
        phinew = rods[i]->get_angle()+dt*omega;


        a_ends[0]=xnew-alength*0.5*cos(phinew);
        a_ends[1]=ynew-alength*0.5*sin(phinew);
        a_ends[2]=xnew+alength*0.5*cos(phinew);
        a_ends[3]=ynew+alength*0.5*sin(phinew);
        
        //Calculate the sheared simulation bounds (at this height)
        xleft  = max(-fov[0] * 0.5 + gamma * a_ends[1] * t, -fov[0] * 0.5 + gamma * a_ends[3] * t);
        xright = min( fov[0] * 0.5 + gamma * a_ends[1] * t,  fov[0] * 0.5 + gamma * a_ends[3] * t);
       //cout<<"\nDEBUG: (xleft, xright) = ( "<<xleft<<" , "<<xright<<" )";
       //cout<<"\nDEBUG: (yleft, yright) = ( "<<yleft<<" , "<<yright<<" )";
        if(this->get_BC() == "REFLECTIVE")
        {
            if (a_ends[0]<=xleft || a_ends[0]>=xright || a_ends[2]<=xleft || a_ends[2]>=xright)
            {
                vx=-vx;
                omega=-omega;

            }
            if (a_ends[1]<=yleft || a_ends[1]>=yright || a_ends[3]<=yleft || a_ends[3]>=yright)
            {
                vy=-vy;
                omega=-omega;
            }

            xnew=rods[i]->get_xcm()+dt*vx;
            ynew=rods[i]->get_ycm()+dt*vy;
           //cout<<"\nDEBUG: after boundary conditions: rod "<<i<<" (xnew, ynew) = ( "<<xnew<<" , "<<ynew<<" )"; 
            phinew=rods[i]->get_angle()+dt*omega;
        }
        else if(this->get_BC() == "PERIODIC")
        {
            if (xnew < xleft) 
            {
                xnew += fov[0];
            }
            else if (xnew > xright)
            {
                xnew -= fov[0];
            }
            
            if (ynew < yleft)
            {
                ynew += fov[1];
            }
            else if(ynew > yright)
            {
                ynew -= fov[1];
            }
        }
        else if(this->get_BC() == "NONE")
        {
            recenter_filament = true;
        }
        
   /*     if (xnew!=xnew){
            cout<<"\nDEBUG: xnew is a nan at update time."<<t<<"  vx = "<<vx;
        }*/
        rods[i]->set_xcm(xnew);
        rods[i]->set_ycm(ynew);
        rods[i]->set_phi(phinew);
        rods[i]->update(); //updates all derived quantities (e.g., endpoints, forces = 0, etc.)
    }
    if (recenter_filament && rods.size() > 0){
        double midx = rods[floor(rods.size()/2)]->get_xcm();
        double midy = rods[floor(rods.size()/2)]->get_ycm();
        for (unsigned int i = 0; i < rods.size(); i++){
            //cout<<"\nDEBUG: recentering rod "<<i<<"to ("<<midx<<" , "<<midy<<")";
            xnew = rods[i]->get_xcm()-midx;
            ynew = rods[i]->get_ycm()-midy;
            /*
            if (xnew!=xnew){
                cout<<"\nDEBUG: xnew is a nan at BC time. midx = "<<midx;
            }*/
            rods[i]->set_xcm(xnew);
            rods[i]->set_ycm(ynew);
            
            rods[i]->update();
       
            }
    }

}

vector<filament *> filament::update_stretching()
{
    vector<filament *> newfilaments;
    
    if(lks.size() == 0)
        return newfilaments;
    
    array<double,4> end_forces0, end_forces1;
    array<double,3> mid_forces;
    
    lks[0]->step();
    end_forces0 = lks[0]->get_forces();
    
    for (unsigned int i=1; i < lks.size(); i++) {
//        if (lks[i]->get_xcm()!=lks[i]->get_xcm()) cout<<"\nDEBUG: lks["<<i<<"]->get_xcm() is inf";
        lks[i]->step();
        if (fabs(lks[i]->get_stretch_force()) > fracture_force){
            
//            cout<<"\nDEBUG: position of rods "<<i<<" and "<<i+1<<": ( "<<rods[i]->get_end()[0]<<" , "<<rods[i]->get_end()[1]<<" ) and ( "<<rods[i+1]->get_start()[0]<<" , "<<rods[i+1]->get_start()[1]<<" ) ";
//            cout<<"\nDEBUG: cm of rods "<<i<<" and "<<i+1<<": ( "<<rods[i]->get_xcm()<<" , "<<rods[i]->get_ycm()<<" ) and ( "<<rods[i+1]->get_xcm()<<" , "<<rods[i+1]->get_ycm()<<" ) ";
//            cout<<"\nDEBUG: distance between rods = "<<pow(rods[i+1]->get_start()[0]-rods[i]->get_end()[0],2) + pow(rods[i+1]->get_start()[1]-rods[i]->get_end()[1],2);
            newfilaments = this->fracture(i);
            break;
        }
        else
        {
        
            end_forces1 = lks[i]->get_forces();
           //cout<<"\nDEBUG: rod "<<i-1<<" end forces : ( "<<end_forces0[2]<<" , "<<end_forces0[3]<<" , "<<end_forces1[0]<<" , "<<end_forces1[1]<<" ) ";
            mid_forces  = this->endForces2centerForce(i-1, end_forces0[2], end_forces0[3], end_forces1[0], end_forces1[1]);
           
            /*
            if (mid_forces[0] != mid_forces[0] || mid_forces[1] != mid_forces[1] || mid_forces[2] != mid_forces[2])
                cout<<"\nDEBUG: encountered inf STRETCHING force at link "<<i;
            */
            rods[i-1]->update_force(mid_forces[0], mid_forces[1], mid_forces[2]);
           //cout<<"\nDEBUG: rod "<<i-1<<" mid forces : ( "<<mid_forces[0]<<" , "<<mid_forces[1]<<" , "<<mid_forces[2]<<" ) ";
             
            end_forces0 = end_forces1;
        }
    }
    
    return newfilaments;
}


actin * filament::get_rod(int i)
{
//    cout<<"\nDEBUG: returning pointer "<<rods[i];
    try
    {
        return rods[i];
    }
    catch (int e)
    {
        cout<<"\nDEBUG: an exception occured while returning the rods[ "<<i<<"]";
        actin * a;
        return a;
    }
}

Link * filament::get_link(int i)
{
    return lks[i];
}

void filament::update_shear(){
    
    double ycm, forcex, force_par, force_perp, lft_trq, rt_trq;
    
    for (unsigned int i = 0; i < rods.size(); i++){
        
        ycm = rods[i]->get_ycm();
        forcex = gamma * ycm;
        
        if (i == 0)
            lft_trq = 0;
        else
            lft_trq = -1 * forcex * (ycm - lks[i]->get_ycm()); //cross product with fy = 0

        if (i == rods.size() - 1)
            rt_trq = 0;
        else{
            rt_trq = -1 * forcex * (ycm - lks[i+1]->get_ycm());
        }
        
        force_par   =  forcex*rods[i]->get_direction()[0];
        force_perp  = -forcex*rods[i]->get_direction()[1];
        cout<<"\nDEBUG: Shearing with (fx, fy, trq) = ("<<force_par<<" , "<<force_perp<<" , "<<lft_trq+rt_trq<<")"; 
        rods[i]->update_force(force_par, force_perp, lft_trq + rt_trq);
    }

}

void filament::update_forces(int index, double f1, double f2, double f3)
{
    rods[index]->update_force(f1,f2,f3);
}

void filament::set_shear(double g){

    gamma = g;

}

string filament::write_rods(){
    string all_rods;
    for (unsigned int i =0; i < rods.size(); i++)
    {
        all_rods += rods[i]->write();
    }

    return all_rods;
}

string filament::write_links(){
    string all_links;
    for (unsigned int i =0; i < lks.size(); i++)
    {
        all_links += lks[i]->write();
    }

    return all_links;
}

vector<actin *> filament::get_rods(unsigned int first, unsigned int last)
{
    vector<actin *> newrods;
    for (unsigned int i = first; i < last; i++)
    {
        if (i < 0 || i >= rods.size())
            break;
        else
            newrods.push_back(new actin(*(rods[i])));
       // cout<<"\nDEBUG: pushing onto stack rods["<<i<<"] : "<<rods[i]->write();
    }
    return newrods;
}

vector<filament *> filament::fracture(int node){

    vector<filament *> newfilaments;
    cout<<"\n\tDEBUG: fracturing at node "<<node;

    newfilaments.push_back(
            new filament(this->get_rods(0,node), lks[0]->get_length(), lks[0]->get_kl(), lks[0]->get_kb(), 
                dt, temperature, fracture_force, gamma, BC));
    newfilaments.push_back(
            new filament(this->get_rods(node, rods.size()), lks[0]->get_length(), lks[0]->get_kl(), lks[0]->get_kb(), 
                dt, temperature, fracture_force, gamma, BC));

    return newfilaments;

}

bool filament::operator==(const filament& that){
    
    for (unsigned int i = 0; i < rods.size(); i++)
        if (!(*(rods[i]) == *(that.rods[i])))
            return false;
    
    for (unsigned int i = 0; i < lks.size(); i++)
        if (!(lks[i]->is_similar(*(that.lks[i]))))
            return false;

    return (this->fov[0] == that.fov[0] && this->fov[1] == that.fov[1] && 
            this->nq[0] == that.nq[0] && this->nq[1] == that.nq[1] &&
            this->gamma == that.gamma && this->temperature == that.temperature &&
            this->dt == that.dt && this->fracture_force == that.fracture_force);

}

string filament::to_string(){
    
    // Note: not including links in to_string, because link's to_string includes filament's to_string
    char buffer[200];
    string out = "";

    for (unsigned int i = 0; i < rods.size(); i++)
        out += rods[i]->to_string();

    sprintf(buffer, "fov = (%f, %f)\tnq = (%d, %d)\tgamma = %f\ttemperature = %f\tdt = %f\tfracture_force=%f\n",
            fov[0], fov[1], nq[0], nq[1], gamma, temperature, dt, fracture_force);
   
    return out + buffer; 

}

string filament::get_BC(){
    return BC; 
}

void filament::set_BC(string s){
    this->BC = s;
}
    
vector<vector<double> *>* filament::fwd_bending_calc()
{
    //Calculate the force at each Link position as outlined by 
    // Allen and Tildesley in Computer Simulation of Liquids, Appendix C
    // Here we let db = (xb, yb) = (linkx[b] - linkx[b-1], linky[b] - linky[b-1])
 
    vector<double> * node_forces_x = new vector<double>();
    vector<double> * node_forces_y = new vector<double>();
    vector<vector<double> *>* node_forces_xy = new vector<vector<double>* >();

    double xam1, xa, xap1, xap2;
    double yam1, ya, yap1, yap2;
    double Cam1am1, Caa, Cap1ap1, Cap2ap2, Caam1, Caap1, Cap1ap2;
    double theta_a, theta_ap1, theta_ap2;
    double coef1, coef2, coef3;
    double fx1, fx2, fx3, fy1, fy2, fy3;
    
    double k = lks[0]->get_kb();
    double forcex, forcey;
    //initialize all NODE forces to be 0
    
    //single rod --> no bending energy 
    if (rods.size() > 1)
    { 
        //First two rods won't have any bending forces: 
        node_forces_x->push_back(0);
        node_forces_y->push_back(0);
        node_forces_x->push_back(0);
        node_forces_y->push_back(0);

        // More than 1 rod--> bending forces to calculate
        xam1      = lks[1]->get_xcm() - lks[0]->get_xcm();
        yam1      = lks[1]->get_ycm() - lks[0]->get_ycm();

        xa        = lks[2]->get_xcm() - lks[1]->get_xcm();
        ya        = lks[2]->get_ycm() - lks[1]->get_ycm();

        Cam1am1 = xam1*xam1 + yam1*yam1;
        Caa     = xa  *xa   + ya  *ya;
        Caam1   = xam1*xa   + yam1*ya;

        theta_a   = rods[1]->get_angle() - rods[0]->get_angle();

        if( fabs(theta_a) < maxSmallAngle )
            coef1 = -k * ( 1 / sqrt( Cam1am1 * Caa     ) ); 
        else
            coef1 = -k*theta_a   / sin(theta_a)   * ( 1 / sqrt( Cam1am1 * Caa     ) ); 

        //2 rods --> bending force on last link; only has one term
        if (rods.size() == 2)
        {

            fx1 = coef1 * ( Caam1/Caa * xa - xam1 );
            fy1 = coef1 * ( Caam1/Caa * ya - yam1 );
            //cout<<"\nDEBUG: magnitude of bending forces at links: ( "<<fx1<<" , "<<fy1<<" )";
            
            if (fabs(fx1) < eps)
                fx1 = 0;
            if (fabs(fy1) < eps)
                fy1 = 0;
            node_forces_x->push_back(fx1);
            node_forces_y->push_back(fy1);

        }
        else
        {
            //More than 2 rods--> more bending forces to calculate
            xap1 = lks[3]->get_xcm() - lks[2]->get_xcm();
            yap1 = lks[3]->get_ycm() - lks[2]->get_ycm();

            Cap1ap1 = xap1*xap1 + yap1*yap1;
            Caap1   = xap1*xa   + yap1*ya;

            theta_ap1 = rods[2]->get_angle() - rods[1]->get_angle();

            if ( fabs(theta_ap1) < maxSmallAngle )
                coef2 =  k * ( 1 / sqrt( Caa     * Cap1ap1 ) );
            else
                coef2 =  k*theta_ap1 / sin(theta_ap1) * ( 1 / sqrt( Caa     * Cap1ap1 ) );

            //Enter loop if more than 3 rods. For 3 rod case, the loop is skipped
            for (unsigned int j = 2; j < lks.size() - 2; j++){

                xap2 = lks[j+2]->get_xcm() - lks[j+1]->get_xcm();
                yap2 = lks[j+2]->get_ycm() - lks[j+1]->get_ycm();

                Cap2ap2 = xap2*xap2 + yap2*yap2;
                Cap1ap2 = xap1*xap2 + yap1*yap2;

                theta_ap2 = rods[j+1]->get_angle() - rods[j]->get_angle();
                
                if ( fabs(theta_ap2) < maxSmallAngle )
                    coef3 = -k * ( 1 / sqrt( Cap1ap1 * Cap2ap2 ) );
                else
                    coef3 = -k*theta_ap2 / sin(theta_ap2) * ( 1 / sqrt( Cap1ap1 * Cap2ap2 ) );
                
                fx1 = coef1 * ( Caam1/Caa * xa - xam1 );
                fy1 = coef1 * ( Caam1/Caa * ya - yam1 );

                fx2 = coef2 * ( (1 + Caap1/Cap1ap1) * xap1 - (1 + Caap1/Caa) * xa ); 
                fy2 = coef2 * ( (1 + Caap1/Cap1ap1) * yap1 - (1 + Caap1/Caa) * ya ); 

                fx3 = coef3 * ( xap2 - Cap1ap2/Cap1ap1 * xap1);
                fy3 = coef3 * ( yap2 - Cap1ap2/Cap1ap1 * yap1);

                forcex = fx1 + fx2 + fx3;
                forcey = fy1 + fy2 + fy3;
                if (fabs(forcex) < eps)
                    forcex = 0;
                if (fabs(forcey) < eps)
                    forcey = 0;

                node_forces_x->push_back(forcex);
                node_forces_y->push_back(forcey);

                //increment all variables for next iteration:
                xam1 = xa;
                xa   = xap1;
                xap1 = xap2;

                yam1 = ya;
                ya   = yap1;
                yap1 = yap2;

                Cam1am1 = Caa;
                Caa     = Cap1ap1;
                Cap1ap1 = Cap2ap2;

                Caam1 = Caap1;
                Caap1 = Cap1ap2;

                coef1 = -1*coef2;
                coef2 = -1*coef3;

            }

            //LAST TWO RODS ON THE FILAMENT:
            fx1 = coef1 * ( Caam1/Caa * xa - xam1 );
            fy1 = coef1 * ( Caam1/Caa * ya - yam1 );
            fx2 = coef2 * ( (1 + Caap1/Cap1ap1) * xap1 - (1 + Caap1/Caa) * xa ); 
            fy2 = coef2 * ( (1 + Caap1/Cap1ap1) * yap1 - (1 + Caap1/Caa) * ya ); 
            
            //cout<<"\nDEBUG: 2nd to last rod: (fx1, fy1) + (fx2, fy2) = ( "<<fx1<< " , "<<fy1<<" ) + ( "<<fx2<<" , " <<fy2<<" )";
            
            forcex = fx1 + fx2;
            forcey = fy1 + fy2;
            if (fabs(forcex) < eps)
                forcex = 0;
            if (fabs(forcey) < eps)
                forcey = 0;
            
            node_forces_x->push_back(forcex);
            node_forces_y->push_back(forcey);

            /*INCREMENT*/
            xam1 = xa;
            xa   = xap1;

            yam1 = ya;
            ya   = yap1;

            Caa   = Cap1ap1;
            Caam1 = Caap1;

            coef1 = -1*coef2;

            fx1 = coef1 * ( Caam1/Caa * xa - xam1 );
            fy1 = coef1 * ( Caam1/Caa * ya - yam1 );
            //cout<<"\nDEBUG: Last rod: (fx1, fy1) = ( "<<fx1<< " , "<<fy1<<" )";

            if (fabs(fx1) < eps)
                fx1 = 0;
            if (fabs(fy1) < eps)
                fy1 = 0;
            
            node_forces_x->push_back(fx1);
            node_forces_y->push_back(fy1);

        }
    }
    node_forces_xy->push_back(node_forces_x);
    node_forces_xy->push_back(node_forces_y);
    return node_forces_xy;

}


//Calculate the forces at each center of mass of the segments
//Update the monomer
void filament::update_bending()
{
    if(lks.size() <= 2)
        return;

    vector<vector<double>* >* fwd_bending = this->fwd_bending_calc();
    array<double,3> forces;

    for (unsigned int j = 0; j < rods.size(); j++){
        forces = this->endForces2centerForce(j, fwd_bending->at(0)->at(j),   fwd_bending->at(1)->at(j), 
                                                fwd_bending->at(0)->at(j+1), fwd_bending->at(1)->at(j+1) );
        /*
        if (forces[0] != forces[0] || forces[1] != forces[1] || forces[2] != forces[2])
            cout<<"\nDEBUG: encountered inf BENDING force at rod "<<j;
        */
        rods[j]->update_force(forces[0], forces[1], forces[2]);
                
    }
    
    delete fwd_bending->at(1);
    delete fwd_bending->at(0);
    delete fwd_bending;

}

/******************************************************************************
 * Change of coordinates function                                             *
 * Takes forces in x and y directions at rod ends                             *
 * i.e, (fx0, fy0) and (fx1, fy1) at end  coordinates (x0, y0), (x1, y1)      *
 * aroudn a rod with center of mass located at (x,y)                          *
 * and applies them to the rod's center of mass as (fx, fy, trq),             *
 * where the torque is around the center of mass                              *
 * ***************************************************************************/

array<double,3> filament::endForces2centerForce(int j, double fx0, double fy0, double fx1, double fy1)
{
    double f0_par, f0_perp, f1_par, f1_perp;
    double x0dot, y0dot, x1dot, y1dot, xdot, ydot, phidot, v_par, v_perp;   
    double x0, y0, x1, y1, dx, dy;     
    
    array<double,3> fric;
    array<double,2> e;
    array<double,3> forces;

    fric = rods[j]->get_frictions();
    e = rods[j]->get_direction();

    /*
    x0 =  lks[j]->get_xcm();
    y0 =  lks[j]->get_ycm();
        
    x1 = lks[j+1]->get_xcm();
    y1 = lks[j+1]->get_ycm();
*/
    x0 = rods[j]->get_start()[0];
    y0 = rods[j]->get_start()[1];
    
    x1 = rods[j]->get_end()[0];
    y1 = rods[j]->get_end()[1];

    f0_par  = fx0 * e[0] + fy0 * e[1];
    f0_perp =-fx0 * e[1] + fy0 * e[0];

    f1_par  = fx1 * e[0] + fy1 * e[1];
    f1_perp =-fx1 * e[1] + fy1 * e[0];

    x0dot = f0_par*e[0]/fric[0] - f0_perp*e[1]/fric[1];
    x1dot = f1_par*e[0]/fric[0] - f1_perp*e[1]/fric[1];

    y0dot = f0_par*e[1]/fric[0] + f0_perp*e[0]/fric[1];
    y1dot = f1_par*e[1]/fric[0] + f1_perp*e[0]/fric[1];

    xdot = (x0dot + x1dot) / 2;
    ydot = (y0dot + y1dot) / 2;
    //cout<<"\nDEBUG : (forcex, forcey) = ("<<forcex<<" , "<<forcey<<" )";

    v_par   =  xdot * e[0] + ydot * e[1];
    v_perp  = -xdot * e[1] + ydot * e[0];

    dx = x1 - x0;
    dy = y1 - y0;

    // phi = arctan[(y1-y0) /(x1-x0)]... take the derivative and you get:
    phidot = (dx*(y1dot-y0dot)-dy*(x1dot-x0dot))/(dx*dx+dy*dy);
    forces[0] = v_par  * fric[0];
    forces[1] = v_perp * fric[1];
    forces[2] = phidot * fric[2];
    
    return forces; 
}


int filament::get_nrods(){
    return rods.size();
}

double filament::get_bending_energy(){
    if(rods.size() == 0){
        return 0;
    }
    
    double sum = 0, theta;

    for (unsigned int i = 0; i < rods.size() - 1; i++)
    {
        theta = rods[i+1]->get_angle() - rods[i]->get_angle();
        sum += theta*theta;
    }
    
    return lks[0]->get_kb()*sum/2.0;

}

double filament::get_stretching_energy(){
    if(rods.size() == 0){
        return 0;
    }
    
    double sum = 0, stretch;

    for (unsigned int i = 0; i < lks.size(); i++)
    {
        stretch = lks[i]->get_stretch_force();
        sum += stretch*stretch;
    }
    
    return sum/(2.0*lks[0]->get_length());

}

double filament::get_total_energy()
{
    return this->get_stretching_energy() + this->get_bending_energy();
}

/////////////////////////////////////
//DOUBLY LINKED FILAMENT FUNCTIONS///
/////////////////////////////////////

DLfilament::DLfilament(double startx, double starty, double startphi, int nrod, double fovx, double fovy, int nqx, int nqy, 
        double visc, double deltat, double temp, bool isStraight,
        double rodLength, double linkLength, double stretching_stiffness, double bending_stiffness,
        double frac_force, double bending_frac_force, string bdcnd) 
    : filament(startx, starty, startphi, nrod, fovx, fovy, nqx, nqy, 
        visc, deltat, temp, isStraight,
        rodLength, linkLength, stretching_stiffness, bending_stiffness,
        frac_force, bdcnd), bending_fracture_force(bending_frac_force)
{
    for (int j = 0; j < (int)rods.size() - 1; j++) {
        midlks.push_back( new MidLink(rodLength + linkLength, stretching_stiffness, bending_stiffness, this, j, j + 1) );  
    }
}

DLfilament::DLfilament(vector<actin *> rodvec, double linkLength, double stretching_stiffness, double bending_stiffness, 
        double deltat, double temp, double frac_force, double bending_frac_force, double g, string bdcnd)
    : filament(rodvec, linkLength, stretching_stiffness, bending_stiffness, 
        deltat, temp, frac_force, g, bdcnd), bending_fracture_force(bending_frac_force)
{
    
    if (rods.size() > 0)
    {
        double rodLength = rods[0]->get_length();
        for (unsigned int j = 0; j < rods.size() - 1; j++) {
            midlks.push_back( new MidLink(rodLength + linkLength, stretching_stiffness, bending_stiffness, this, j, j + 1) );  
        }
    }

}

void DLfilament::set_bending_linear()
{
    for (unsigned int j = 0; j < midlks.size(); j++)
    {
        midlks[j]->set_linear(true);
    }
}

vector<DLfilament *> DLfilament::update_bending()
{
    vector<DLfilament *> newfilaments;
    for (unsigned int i=0; i < midlks.size(); i++) {
        midlks[i]->step();
        if (fabs(midlks[i]->get_stretch_force()) > bending_fracture_force){
            newfilaments = this->fracture(i);
            break;
        }
        else{
            midlks[i]->filament_update();
        }
    }
    return newfilaments;
}

vector<DLfilament *> DLfilament::update_stretching()
{
    vector<DLfilament *> newfilaments;
    for (unsigned int i=0; i < lks.size(); i++) {
        if (fabs(lks[i]->get_stretch_force()) > fracture_force){
            
            newfilaments = this->fracture(i);
            break;
        }
        else{
            lks[i]->step();
            lks[i]->filament_update();
        }
    }
    return newfilaments;
}

vector<DLfilament *> DLfilament::fracture(int node)
{

    vector<DLfilament *> newfilaments;
    cout<<"\n\tDEBUG: fracturing at node "<<node;

    newfilaments.push_back(
            new DLfilament(this->get_rods(0,           node - 1), lks[0]->get_length(), lks[0]->get_kl(), lks[0]->get_kb(), 
                dt, temperature, fracture_force, bending_fracture_force, gamma, BC));
    newfilaments.push_back(
            new DLfilament(this->get_rods(node, rods.size() - 1), lks[0]->get_length(), lks[0]->get_kl(), lks[0]->get_kb(), 
                dt, temperature, fracture_force, bending_fracture_force, gamma, BC));

    return newfilaments;

}

DLfilament::~DLfilament(){
    int nr = rods.size(), nl = lks.size(), nml = midlks.size();
    for (int i = 0; i < nr; i ++)
        delete rods[i];
    for (int i = 0; i < nl; i ++)
        delete lks[i];
    for (int i = 0; i < nml; i++)
        delete midlks[i];
    
    rods.clear();
    lks.clear();
    midlks.clear();
}


