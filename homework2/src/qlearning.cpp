#include "../include/qlearning.hpp"


namespace cleaner{
    qlearning::qlearning(world const& w, double epsilon, double learning_rate, double gamma, int episodes) : w(w), episodes(episodes), gamma(gamma), epsilon(epsilon), learning_rate(learning_rate){
    }

    qlearning::~qlearning(){
    }

    // For each new episode, print the mean gain value
    // (sum of gains since the beginning of the experiment) for state 0
    void qlearning::plots(){
      std::cout << this->getGainAt() << std::endl;
      points.push_back(std::make_pair(this->episode, this->getGainAt()));

      gp << "set xlabel 'Episodes'\n";
      gp << "set ylabel 'Mean Gain Value'\n";
      gp << "plot '-' binary" << gp.binFmt1d(points, "record") << "with lines title 'Q-learning'\n";
      gp.sendBinary1d(points);
      gp.flush();
  }


    void qlearning::solve(){
      double r;
      int s, a, ss;
      this->init();
      double sumRewards;
      double finalGain = 0;
      do{
        s=0;
        sumRewards = 0.0;
        for(int i=0; i<100; i++){
          a = greedy(s);
          w.execute(s, static_cast<action>(a), ss, r);
          this->backup(s,a,ss,r);
          s = ss;
          sumRewards += r;
        }
        this->G.push_back(sumRewards);
        this->plots();

      }while( ++this->episode < this->episodes );
      for(int i = 0; i < this->episodes; i++){
        finalGain += G[i];
      }
      printf("Final mean gain = %f\n", finalGain / this->episodes);
    }

    // Get the current mean gain for state 0
    double qlearning::getGainAt(){
      double gain = 0.0;
      for(int i = 0; i < this->episode; i++){
        gain += G[i];
      }
      return gain / this->episode;
    }

    double qlearning::getValueAt(int s){
      double value = MIN;
      for(int a=0; a<action::END; ++a){
        value = std::max(value, this->getScalar(s, a));
      } return value;
    }

    int qlearning::greedy(int s){
      int agreedy;
      double value = MIN;
      double rd = rand() / ((double) RAND_MAX);

      if( rd > this->epsilon ) {
        for(int a=0; a<action::END; ++a){
          if( value < this->getScalar(s, a)){
            agreedy = a;
            value = this->getScalar(s, a);
          }
        }
      }

      else {
        agreedy = rand() % 7;
      }

      return agreedy;
    }

    // Update the value of theta
    void qlearning::backup(int s, int a, int ss, double r){
      std::vector<double> p = defPhi(s,a);
      for(int i = 0; i < this->nb_pi; i++){
        this->theta[i] += this->learning_rate * (r + this->gamma*getValueAt(ss) - this->getScalar(s,a)) * p[i];
      }
    }

    void qlearning::init(){
      this->nb_pi = 7;
      for(int i=0; i<this->nb_pi; i++){
        this->theta.push_back(0.0);
      }

    }

    // Compute the phi vector
    std::vector<double> qlearning::defPhi(int s, int a){

      std::vector<double> p;
      for(int i = 0; i < this->nb_pi; i++){
        p.push_back(-1.0);
      }
      // Si on est sur la base, on veut que le robot se recharge
      if(w.getState(s)->getBase() && w.getState(s)->getBattery() < w.getCBattery() && a  == action::CHARGE){
        p[0]= 0.1;
      }
      // Si on a juste assez de batterie pour revenir à la base, on revient
      // TODO: Position base?
      if((int(w.getState(s)->getBase()) / w.getHeight() + int(w.getState(s)->getBase()) % w.getWidth() == w.getState(s)->getBattery()) && (a  == action::LEFT || a  == action::UP)){
        p[1]= 0.1;
      }
      // Si case sale, on nettoie
      if(!w.getGrid(w.getState(s)->getGrid(), w.getState(s)->getPose()) && a == action::CLEAN){
        p[2]= 0.1;
      }

      bool condition = true;
      // ! Si pas de mur à gauche ou case de gauche est clean
      if(w.getState(s)->getPose() > 0){
        if(!(w.getState(s)->getPose() % w.getWidth() == 0 /*|| !w.getGrid(w.getState(s)->getGrid(), w.getState(s)->getPose() - 1)*/) && a == action::LEFT){
          p[3]= 0.1;
          condition = false;
        }
      }
      // ! Si pas de mur en haut ou case en haut est clean
      if(w.getState(s)->getPose() > (w.getWidth() - 1) && condition){
        if( !(w.getState(s)->getPose() % w.getHeight() == 0 /*|| !w.getGrid(w.getState(s)->getGrid(), w.getState(s)->getPose() - w.getWidth())*/) && a == action::UP ){
          p[4]= 0.1;
          condition = false;
        }
      }
      // ! Si pas de mur à droite ou case à droite est clean
      if(w.getState(s)->getPose() < w.getHeight() * w.getWidth() - 1 && condition){
        if(!(w.getState(s)->getPose() % w.getWidth() == w.getWidth() /*|| !w.getGrid(w.getState(s)->getGrid(), w.getState(s)->getPose() + 1)*/) && a == action::RIGHT ){
          p[5]= 0.1;
          condition = false;
        }
      }
      // ! Si pas de mur en bas ou case en bas est clean
      if(w.getState(s)->getPose() < ((w.getWidth()*w.getHeight())-(w.getWidth() - 1)) && condition){
        if( !(w.getState(s)->getPose() % w.getHeight() == 0 /*|| !w.getGrid(w.getState(s)->getGrid(), w.getState(s)->getPose() + w.getWidth())*/) && a == action::UP ){
          p[6]= 0.1;
          condition = false;
        }
      }
      if(condition){
        p[7] = 1;
      }
      /*// ! Si que des murs et des cases nettoyées autour, case sale la plus proche
      else if((s.getPose() % w.getWidth() == 0 || s.getGrid(s.getPose()-1)) && (s.getPose() % w.getHeight() == 0 || s.getGrid(s.getPose()-w.getWidth())) && (s.getPose() % w.getWidth() == w.getWidth() || s.getGrid(s.getPose()+1)) && (s.getPose() % w.getHeight() == w.getHeight() || s.getGrid(s.getPose()+w.getWidth()))) {
        action a = NearestDirtyDirection();
        if( a == action:LEFT ){
          p[7] = 10.0;
        }
        else if ( a == action:UP ){
          p[8] = 10.0;
        }
        else if ( a == action:RIGHT ){
          p[9] = 10.0;
        }
        else if ( a == action:DOWN ){
          p[10] = 10.0;
        }
      }*/
      return p;
    }

    // Get the result of theta * phi
    double qlearning::getScalar(int s, int a){
      double result = 0.0;
      std::vector<double> p = defPhi(s,a);
      for(int i = 0; i < this->nb_pi; i++){
          result+=this->theta[i] * p[i];
      }
      return result;
    }
}
