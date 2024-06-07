#include "AIPlayer.h"
#include "Parchis.h"

const double masinf = 9999999999.0, menosinf = -9999999999.0;
const double gana = masinf - 1, pierde = menosinf + 1;
const int num_pieces = 3;
const int PROFUNDIDAD_MINIMAX = 4;  // Umbral maximo de profundidad para el metodo MiniMax
const int PROFUNDIDAD_ALFABETA = 6; // Umbral maximo de profundidad para la poda Alfa_Beta

bool AIPlayer::move()
{
    cout << "Realizo un movimiento automatico" << endl;

    color c_piece;
    int id_piece;
    int dice;
    think(c_piece, id_piece, dice);

    cout << "Movimiento elegido: " << str(c_piece) << " " << id_piece << " " << dice << endl;

    actual->movePiece(c_piece, id_piece, dice);
    return true;
}
// CODIGO TUTORIAL
void AIPlayer::thinkAleatorio(color &c_piece, int &id_piece, int &dice) const
{
    // El id de mi jugador actual.
    int player = actual->getCurrentPlayerId();

    // Vector que almacenará los dados que se pueden usar para el movimiento
    vector<int> current_dices;
    // Vector que almacenará los ids de las fichas que se pueden mover para el dado elegido.
    vector<tuple<color, int>> current_pieces;

    // Se obtiene el vector de dados que se pueden usar para el movimiento
    current_dices = actual->getAvailableNormalDices(player);
    // Elijo un dado de forma aleatoria.
    dice = current_dices[rand() % current_dices.size()];

    // Se obtiene el vector de fichas que se pueden mover para el dado elegido
    current_pieces = actual->getAvailablePieces(player, dice);

    // Si tengo fichas para el dado elegido muevo una al azar.
    if (current_pieces.size() > 0)
    {
        int random_id = rand() % current_pieces.size();
        id_piece = get<1>(current_pieces[random_id]); // get<i>(tuple<...>) me devuelve el i-ésimo
        c_piece = get<0>(current_pieces[random_id]);  // elemento de la tupla
    }
    else
    {
        // Si no tengo fichas para el dado elegido, pasa turno (la macro SKIP_TURN me permite no mover).
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor(); // Le tengo que indicar mi color actual al pasar turno.
    }
}
void AIPlayer::thinkAleatorioMasInteligente(color &c_piece, int &id_piece, int &dice) const
{
    int player = actual->getCurrentPlayerId();
    vector<int> current_dices;
    vector<tuple<color, int>> current_pieces;
    current_dices = actual->getAvailableNormalDices(player);
    vector<int> current_dices_que_pueden_mover_ficha;
    for (int i = 0; i < current_dices.size(); i++)
    {
        current_pieces = actual->getAvailablePieces(player, current_dices[i]);
        if (current_pieces.size() > 0)
        {
            current_dices_que_pueden_mover_ficha.push_back(current_dices[i]);
        }
    }
    if (current_dices_que_pueden_mover_ficha.size() == 0)
    {
        dice = current_dices[rand() % current_dices.size()];
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
    }
    else
    {
        dice = current_dices_que_pueden_mover_ficha[rand() % current_dices_que_pueden_mover_ficha.size()];
        current_pieces = actual->getAvailablePieces(player, dice);
        int random_id = rand() % current_pieces.size();
        id_piece = get<1>(current_pieces[random_id]);
        c_piece = get<0>(current_pieces[random_id]);
    }
}
void AIPlayer::thinkFichaMasAdelantada(color &c_piece, int &id_piece, int &dice) const
{
    thinkAleatorioMasInteligente(c_piece, id_piece, dice);
    // tras esto, tengo en dice el número de dao a usar.
    // en vez de al azar, elijo la ficha más adelantada.
    int player = actual->getCurrentPlayerId();
    vector<tuple<color, int>> current_pieces = actual->getAvailablePieces(player, dice);
    int id_ficha_mas_adelantada = -1;
    color col_ficha_mas_adelantada = none;
    int min_distancia_meta = 9999;
    for (int i = 0; i < current_pieces.size(); i++)
    {
        // distanceToGoal(color,id) devuelve la distancia de la ficha a la meta del color que le indique.
        color col = get<0>(current_pieces[i]);
        int id = get<1>(current_pieces[i]);
        int distancia_meta = actual->distanceToGoal(col, id);
        if (distancia_meta < min_distancia_meta)
        {
            min_distancia_meta = distancia_meta;
            id_ficha_mas_adelantada = id;
            col_ficha_mas_adelantada = col;
        }
        if (id_ficha_mas_adelantada == -1)
        {
            id_piece = SKIP_TURN;
            c_piece = actual->getCurrentColor();
        }
        else
        {
            id_piece = id_ficha_mas_adelantada;
            c_piece = col_ficha_mas_adelantada;
        }
    }
}
void AIPlayer::thinkMejorOpcion(color &c_piece, int &id_piece, int &dice) const
{
    ParchisBros hijos = actual->getChildren();
    bool me_quedo_con_esta_funcion = false;
    int current_power = actual->getPowerBar(this->jugador).getPower();
    int max_power = -101; // maxima energía

    for (ParchisBros::Iterator it = hijos.begin(); it != hijos.end(); ++it)
    {
        Parchis siguiente_hijo = *it;
        if (siguiente_hijo.isEatingMove() or siguiente_hijo.isGoalMove() or (siguiente_hijo.gameOver() and siguiente_hijo.getWinner() == this->jugador))
        {
            me_quedo_con_esta_funcion = true;
            c_piece = it.getMovedColor();
            id_piece = it.getMovedPieceId();
            dice = it.getMovedDiceValue();
        }
        else
        {
            int new_power = siguiente_hijo.getPower(this->jugador);
            if (new_power - current_power > max_power)
            {
                c_piece = it.getMovedColor();
                id_piece = it.getMovedPieceId();
                dice = it.getMovedDiceValue();
                max_power = new_power - current_power;
            }
        }
    }
}
// FIN CODIGO TUTORIAL


// Implementación Poda Alfa-Beta
double AIPlayer::Poda_AlfaBeta(const Parchis &estado, int jugador, int profundidad, int profundidad_max, color &mejor_color, int &mejor_id, int &mejor_dado, double alpha, double beta, double (*heuristica)(const Parchis &, int)) const
{
    if (profundidad == profundidad_max || estado.gameOver())
    {
        return heuristica(estado, jugador);
    }
    ParchisBros hijos = estado.getChildren();
    if (estado.getCurrentPlayerId() == jugador)
    {
        // Nodo MAX
        double valor_max = menosinf;
        for (ParchisBros::Iterator it = hijos.begin(); it != hijos.end(); ++it)
        {
            Parchis siguiente_hijo = *it;
            color c = it.getMovedColor();
            int id = it.getMovedPieceId();
            int dado = it.getMovedDiceValue();
            double valor = Poda_AlfaBeta(siguiente_hijo, jugador, profundidad + 1, profundidad_max, mejor_color, mejor_id, mejor_dado, alpha, beta, heuristica);
            if (valor > valor_max)
            {
                valor_max = valor;
                if (profundidad == 0)
                {
                    mejor_color = c;
                    mejor_id = id;
                    mejor_dado = dado;
                }
            }
            alpha = max(alpha, valor);
            if (beta <= alpha)
            {
                break; // poda beta
            }
        }
        return valor_max;
    }
    else
    {
        // Nodo MIN
        int oponente = (jugador + 1) % 2;
        double valor_min = masinf;
        for (ParchisBros::Iterator it = hijos.begin(); it != hijos.end(); ++it)
        {
            Parchis siguiente_hijo = *it;
            double valor = Poda_AlfaBeta(siguiente_hijo, jugador, profundidad + 1, profundidad_max, mejor_color, mejor_id, mejor_dado, alpha, beta, heuristica);
            valor_min = min(valor_min, valor);
            beta = min(beta, valor);
            if (beta <= alpha)
            {
                break; // poda alfa
            }
        }
        return valor_min;
    }
}

void AIPlayer::think(color &c_piece, int &id_piece, int &dice) const
{
    double valor, alpha = menosinf, beta = masinf;
    switch (id)
    {
    case 0:
        valor = Poda_AlfaBeta(*actual, jugador, 0, PROFUNDIDAD_ALFABETA, c_piece, id_piece, dice, alpha, beta, ValoracionTest);
        break;
    case 1:
        valor = Poda_AlfaBeta(*actual, jugador, 0, PROFUNDIDAD_ALFABETA, c_piece, id_piece, dice, alpha, beta, MiValoracion1);
        break;
    case 2:
        valor = Poda_AlfaBeta(*actual, jugador, 0, PROFUNDIDAD_ALFABETA, c_piece, id_piece, dice, alpha, beta, MiValoracion2);
    }
}

 // HEURISTICA 1

double AIPlayer::MiValoracion1(const Parchis &estado, int jugador) {
    int ganador = estado.getWinner();
    int oponente = (jugador + 1) % 2;
    const int NUM_CASILLAS = 68 + 7; // Número de casillas hasta la meta

    if (ganador == jugador) {
        return gana;
    } else if (ganador == oponente) {
        return pierde;
    } else {
        vector<color> my_colors = estado.getPlayerColors(jugador);
        vector<color> op_colors = estado.getPlayerColors(oponente);

        double puntuacion_jugador = 0.0;

        if (estado.getCurrentPlayerId() == jugador) {
            if (estado.isEatingMove()) { // si come
                pair<color, int> piezaComida = estado.eatenPiece();
                if (piezaComida.first == my_colors[0] || piezaComida.first == my_colors[1]) {
                    puntuacion_jugador -= 5; // Penaliza menos si come una propia
                } else {
                    puntuacion_jugador += 100; // Valor más alto si come una del oponente
                }
            } else if (estado.isGoalMove()) { // si ha metido ficha
                puntuacion_jugador += 35; // Más puntos por mover una ficha a la meta
            } else {
                auto piezasDestruidas = estado.piecesDestroyedLastMove();
                if (!piezasDestruidas.empty()) { // si ha destruido piezas del rival suma, si suyas resta
                    for (auto it = piezasDestruidas.begin(); it != piezasDestruidas.end(); ++it) {
                        if (it->first == my_colors[0] || it->first == my_colors[1]) {
                            puntuacion_jugador -= 35; // Penaliza si destruye una propia
                        } else {
                            puntuacion_jugador += 35; // Aumenta si destruye una del oponente
                        }
                    }
                } else if (estado.getItemAcquired() != -1) {
                    puntuacion_jugador += 15; // Más puntos por adquirir un objeto
                } else if (estado.goalBounce()) {
                    puntuacion_jugador -= 2; // Penaliza más por rebotar en la meta
                }
            }
        }

        for (color c : my_colors) {
            puntuacion_jugador -= estado.piecesAtHome(c) * 2; // Penaliza más las piezas en casa
            for (int j = 0; j < num_pieces; j++) {
                puntuacion_jugador += NUM_CASILLAS - estado.distanceToGoal(c, j) + estado.piecesAtGoal(c) * 6;
            }
        }

        double puntuacion_oponente = 0.0;

        if (estado.getCurrentPlayerId() == oponente) {
            if (estado.isEatingMove()) {
                pair<color, int> piezaComida = estado.eatenPiece();
                if (piezaComida.first == op_colors[0] || piezaComida.first == op_colors[1]) {
                    puntuacion_oponente -= 5;
                } else {
                    puntuacion_oponente += 100;
                }
            } else if (estado.isGoalMove()) {
                puntuacion_oponente += 35;
            } else {
                auto piezasDestruidas = estado.piecesDestroyedLastMove();
                if (!piezasDestruidas.empty()) {
                    for (auto it = piezasDestruidas.begin(); it != piezasDestruidas.end(); ++it) {
                        if (it->first == op_colors[0] || it->first == op_colors[1]) {
                            puntuacion_oponente -= 35;
                        } else {
                            puntuacion_oponente += 35;
                        }
                    }
                } else if (estado.getItemAcquired() != -1) {
                    puntuacion_oponente += 15;
                } else if (estado.goalBounce()) {
                    puntuacion_oponente -= 2;
                }
            }
        }

        for (color c : op_colors) {
            puntuacion_oponente -= estado.piecesAtHome(c) * 2;
            for (int j = 0; j < num_pieces; j++) {
                puntuacion_oponente += NUM_CASILLAS - estado.distanceToGoal(c, j) + estado.piecesAtGoal(c) * 6;
            }
        }

        return puntuacion_jugador - puntuacion_oponente;
    }
}


double AIPlayer::MiValoracion2(const Parchis &estado, int jugador){
     int ganador = estado.getWinner();
    int oponente = (jugador + 1) % 2;
    const int NUM_CASILLAS = 68 + 7; // Número de casillas hasta la meta
    const double BARRIER_BONUS = 20.0;
    const double MEGA_BARRIER_BONUS = 40.0;
    const double SAFE_BONUS = 5.0;
    const double EAT_BONUS = 100.0;
    const double GOAL_BONUS = 50.0;
    const double PIECE_HOME_PENALTY = -5.0;
    const double PIECE_DESTROYED_PENALTY = -30.0;

    if (ganador == jugador) {
        return gana;
    } else if (ganador == oponente) {
        return pierde;
    } else {
        vector<color> my_colors = estado.getPlayerColors(jugador);
        vector<color> op_colors = estado.getPlayerColors(oponente);

        double puntuacion_jugador = 0.0;
        double energy = estado.getPower(jugador);

        // Valoración del jugador actual
        for (color c : my_colors) {
            puntuacion_jugador -= estado.piecesAtHome(c) * PIECE_HOME_PENALTY; // Penaliza las piezas en casa

            for (int j = 0; j < num_pieces; j++) {
                puntuacion_jugador += NUM_CASILLAS - estado.distanceToGoal(c, j) + estado.piecesAtGoal(c) * GOAL_BONUS;

                Box current_box = estado.getBoard().getPiece(c, j).get_box();
                if (estado.isWall(current_box) == c) {
                    puntuacion_jugador += BARRIER_BONUS; // Bonificación por barrera propia
                }
                if (estado.isMegaWall(current_box) == c) {
                    puntuacion_jugador += MEGA_BARRIER_BONUS; // Bonificación por megabarrera propia
                }
            }
        }

        // Valoración del oponente
        double puntuacion_oponente = 0.0;
        for (color c : op_colors) {
            puntuacion_oponente -= estado.piecesAtHome(c) * PIECE_HOME_PENALTY;

            for (int j = 0; j < num_pieces; j++) {
                puntuacion_oponente += NUM_CASILLAS - estado.distanceToGoal(c, j) + estado.piecesAtGoal(c) * GOAL_BONUS;

                Box current_box = estado.getBoard().getPiece(c, j).get_box();
                if (estado.isWall(current_box) == c) {
                    puntuacion_oponente -= BARRIER_BONUS; // Penalización por barrera del oponente
                }
                if (estado.isMegaWall(current_box) == c) {
                    puntuacion_oponente -= MEGA_BARRIER_BONUS; // Penalización por megabarrera del oponente
                }
            }
        }

        // Efectos del dado especial
        if (estado.getCurrentPlayerId() == jugador) {
            if (estado.isEatingMove()) {
                pair<color, int> piezaComida = estado.eatenPiece();
                if (piezaComida.first != my_colors[0] && piezaComida.first != my_colors[1]) {
                    puntuacion_jugador += EAT_BONUS; // Bonificación por comer una ficha del oponente
                } else {
                    puntuacion_jugador += PIECE_DESTROYED_PENALTY; // Penalización por comer una ficha propia
                }
            } else if (estado.isGoalMove()) {
                puntuacion_jugador += GOAL_BONUS; // Bonificación por llegar a la meta
            }
        }

        // Aplicación de las acciones del dado especial
        if (energy >= 50 && energy < 60 || energy >= 70 && energy < 75) {
            puntuacion_jugador += EAT_BONUS; // Bonificación por concha roja
        } else if (energy >= 60 && energy < 65) {
            puntuacion_jugador += PIECE_DESTROYED_PENALTY; // Penalización por BOOM
        } else if (energy >= 65 && energy < 70) {
            puntuacion_jugador += GOAL_BONUS; // Bonificación por movimiento ultra rápido
        } else if (energy >= 75 && energy < 80) {
            puntuacion_jugador += GOAL_BONUS * 2; // Bonificación por movimiento bala
        } else if (energy >= 80 && energy < 85) {
            puntuacion_jugador += PIECE_DESTROYED_PENALTY; // Penalización por CATAPUM
        } else if (energy >= 85 && energy < 90) {
            puntuacion_jugador += EAT_BONUS; // Bonificación por concha azul
        } else if (energy >= 90 && energy < 95) {
            puntuacion_jugador += PIECE_DESTROYED_PENALTY * 2; // Penalización por BOOMBOOM
        } else if (energy >= 95 && energy < 100) {
            puntuacion_jugador += SAFE_BONUS * 5; // Bonificación por movimiento estrella
        } else if (energy == 100) {
            puntuacion_jugador += PIECE_DESTROYED_PENALTY * 3; // Penalización por CATAPUMCHIMPUM
        }

        return puntuacion_jugador - puntuacion_oponente;
    }
}

double AIPlayer::ValoracionTest(const Parchis &estado, int jugador)
{
    // Heurística de prueba proporcionada para validar el funcionamiento del algoritmo de búsqueda.

    int ganador = estado.getWinner();
    int oponente = (jugador + 1) % 2;

    // Si hay un ganador, devuelvo más/menos infinito, según si he ganado yo o el oponente.
    if (ganador == jugador)
    {
        return gana;
    }
    else if (ganador == oponente)
    {
        return pierde;
    }
    else
    {
        // Colores que juega mi jugador y colores del oponente
        vector<color> my_colors = estado.getPlayerColors(jugador);
        vector<color> op_colors = estado.getPlayerColors(oponente);

        // Recorro todas las fichas de mi jugador
        int puntuacion_jugador = 0;
        // Recorro colores de mi jugador.
        for (int i = 0; i < my_colors.size(); i++)
        {
            color c = my_colors[i];
            // Recorro las fichas de ese color.
            for (int j = 0; j < num_pieces; j++)
            {
                // Valoro positivamente que la ficha esté en casilla segura o meta.
                if (estado.isSafePiece(c, j))
                {
                    puntuacion_jugador++;
                }
                else if (estado.getBoard().getPiece(c, j).get_box().type == goal)
                {
                    puntuacion_jugador += 5;
                }
            }
        }

        // Recorro todas las fichas del oponente
        int puntuacion_oponente = 0;
        // Recorro colores del oponente.
        for (int i = 0; i < op_colors.size(); i++)
        {
            color c = op_colors[i];
            // Recorro las fichas de ese color.
            for (int j = 0; j < num_pieces; j++)
            {
                if (estado.isSafePiece(c, j))
                {
                    // Valoro negativamente que la ficha esté en casilla segura o meta.
                    puntuacion_oponente++;
                }
                else if (estado.getBoard().getPiece(c, j).get_box().type == goal)
                {
                    puntuacion_oponente += 5;
                }
            }
        }

        // Devuelvo la puntuación de mi jugador menos la puntuación del oponente.
        return puntuacion_jugador - puntuacion_oponente;
    }
}