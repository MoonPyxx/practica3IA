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

    // Puntuaciones para diferentes situaciones
    const double BONUS_BARRERA = 20.0;
    const double BONUS_MEGA_BARRERA = 40.0;
    const double BONUS_SEGURO = 5.0;
    const double BONUS_COMER = 100.0;
    const double BONUS_META = 50.0;
    const double PENALIZACION_CASA = -5.0;
    const double PENALIZACION_DESTRUIDA = -30.0;
    const double PENALIZACION_REBOTE = -10.0;
    const double BONUS_OBJETO = 15.0;
    const double BONUS_COMER_PROPIO = 50.0; // Nuevo bonus por comer una ficha propia

    if (ganador == jugador) {
        return gana;
    } else if (ganador == oponente) {
        return pierde;
    } else {
        vector<color> mis_colores = estado.getPlayerColors(jugador);
        vector<color> colores_oponente = estado.getPlayerColors(oponente);

        double puntuacion_jugador = 0.0;
        double energia = estado.getPower(jugador);

        // Evaluación del movimiento reciente del jugador
        if (estado.getCurrentPlayerId() == jugador) {
            if (estado.isEatingMove()) { // Si come ficha
                pair<color, int> piezaComida = estado.eatenPiece();
                if (piezaComida.first == mis_colores[0] || piezaComida.first == mis_colores[1]) {
                    // Solo comer una ficha propia si está cerca de la meta y si es beneficioso
                    int dist_meta = estado.distanceToGoal(piezaComida.first, piezaComida.second);
                    if (dist_meta < 10 && estado.distanceToGoal(piezaComida.first, piezaComida.second) < estado.distanceToGoal(piezaComida.first, piezaComida.second)) {
                        puntuacion_jugador += BONUS_COMER_PROPIO; // Bonificación por comer una propia cerca de la meta
                    } else {
                        puntuacion_jugador -= 5; // Penalización básica por comer una propia
                    }
                } else {
                    puntuacion_jugador += BONUS_COMER; // Valor alto si come una del oponente
                }
            } else if (estado.isGoalMove()) { // Si ha metido ficha
                puntuacion_jugador += BONUS_META; // Más puntos por mover una ficha a la meta
            } else {
                auto piezasDestruidas = estado.piecesDestroyedLastMove();
                if (!piezasDestruidas.empty()) { // Si ha destruido piezas del rival suma, si suyas resta
                    for (auto it = piezasDestruidas.begin(); it != piezasDestruidas.end(); ++it) {
                        if (it->first == mis_colores[0] || it->first == mis_colores[1]) {
                            puntuacion_jugador += PENALIZACION_DESTRUIDA; // Penaliza si destruye una propia
                        } else {
                            puntuacion_jugador -= PENALIZACION_DESTRUIDA; // Aumenta si destruye una del oponente
                        }
                    }
                } else if (estado.getItemAcquired() != -1) {
                    puntuacion_jugador += BONUS_OBJETO; // Más puntos por adquirir un objeto
                } else if (estado.goalBounce()) {
                    puntuacion_jugador += PENALIZACION_REBOTE; // Penaliza más por rebotar en la meta
                }
            }
        }

        // Evaluación del estado global del jugador
        for (color c : mis_colores) {
            puntuacion_jugador += estado.piecesAtHome(c) * PENALIZACION_CASA; // Penaliza las piezas en casa
            puntuacion_jugador += estado.piecesAtGoal(c) * BONUS_META; // Premia las fichas en la meta

            for (int j = 0; j < num_pieces; j++) {
                int dist_meta = estado.distanceToGoal(c, j);
                puntuacion_jugador += (NUM_CASILLAS - dist_meta); // Premia la cercanía a la meta

                if (estado.isSafePiece(c, j)) {
                    puntuacion_jugador += BONUS_SEGURO; // Premia estar en una casilla segura
                }

                Box ficha_box = estado.getBoard().getPiece(c, j).get_box();
                if (estado.isWall(ficha_box) == c) {
                    puntuacion_jugador += BONUS_BARRERA; // Bonificación por barrera propia
                }

                if (estado.isMegaWall(ficha_box) == c) {
                    puntuacion_jugador += BONUS_MEGA_BARRERA; // Bonificación por megabarrera propia
                }
            }
        }

        // Evaluación del estado global del oponente
        double puntuacion_oponente = 0.0;
        for (color c : colores_oponente) {
            puntuacion_oponente += estado.piecesAtHome(c) * PENALIZACION_CASA;
            puntuacion_oponente += estado.piecesAtGoal(c) * BONUS_META;

            for (int j = 0; j < num_pieces; j++) {
                int dist_meta = estado.distanceToGoal(c, j);
                puntuacion_oponente += (NUM_CASILLAS - dist_meta);

                if (estado.isSafePiece(c, j)) {
                    puntuacion_oponente += BONUS_SEGURO;
                }

                Box ficha_box = estado.getBoard().getPiece(c, j).get_box();
                if (estado.isWall(ficha_box) == c) {
                    puntuacion_oponente += BONUS_BARRERA;
                }

                if (estado.isMegaWall(ficha_box) == c) {
                    puntuacion_oponente += BONUS_MEGA_BARRERA;
                }
            }
        }

        // Aplicación de las acciones del dado especial
        if (energia < 50) {
            puntuacion_jugador += (7 + energia / 7); // Movimiento rápido
        } else if (energia >= 50 && energia < 60) {
            puntuacion_jugador += BONUS_COMER; // Concha roja
        } else if (energia >= 60 && energia < 65) {
            puntuacion_jugador -= PENALIZACION_DESTRUIDA; // BOOM
        } else if (energia >= 65 && energia < 70) {
            puntuacion_jugador += 25; // Movimiento ultra rápido
        } else if (energia >= 70 && energia < 75) {
            puntuacion_jugador += BONUS_COMER; // Concha roja
        } else if (energia >= 75 && energia < 80) {
            puntuacion_jugador += 40; // Movimiento bala
        } else if (energia >= 80 && energia < 85) {
            puntuacion_jugador -= PENALIZACION_DESTRUIDA; // CATAPUM
        } else if (energia >= 85 && energia < 90) {
            puntuacion_jugador += BONUS_COMER; // Concha azul
        } else if (energia >= 90 && energia < 95) {
            puntuacion_jugador -= 2 * PENALIZACION_DESTRUIDA; // BOOMBOOM
        } else if (energia >= 95 && energia < 100) {
            puntuacion_jugador += BONUS_SEGURO * 5; // Movimiento estrella
        } else if (energia == 100) {
            puntuacion_jugador -= 3 * PENALIZACION_DESTRUIDA; // CATAPUMCHIMPUM
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