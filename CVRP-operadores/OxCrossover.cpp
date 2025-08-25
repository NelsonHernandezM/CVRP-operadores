#include "OxCrossover.h"
#pragma once
#include <vector>
#include <numeric>
#include <algorithm>
#include <vector>

 

void OxCrossover::initialize(Requirements* config) {
    this->local_ini = false;
}

void OxCrossover::execute(SolutionSet parents, SolutionSet children) {
    RandomNumber* rnd = RandomNumber::getInstance();
    int n = parents.get(0).getNumVariables();

 

    // Extraer solo los clientes de cada padre
    std::vector<int> clientesPadre1, clientesPadre2;
    clientesPadre1.reserve(n);
    clientesPadre2.reserve(n);

   
    int max_node_id = 0;
    //mayor a CERO debido a que aqui 0 es el depsotio y -1 el final de todas las rutas
    for (int i = 0; i < n; ++i) {
        int nodo1 = parents.get(0).getVariableValue(i).L;
        int nodo2 = parents.get(1).getVariableValue(i).L;

 
        if (nodo1 != 0 && nodo1 != -1) {
            clientesPadre1.push_back(nodo1);
            if (nodo1 > max_node_id) max_node_id = nodo1;
        }
        if (nodo2 != 0 && nodo2 != -1) {
            clientesPadre2.push_back(nodo2);
            if (nodo2 > max_node_id) max_node_id = nodo2;
        }
    }
 
    if (clientesPadre1.size() < 2 || clientesPadre2.size() < 2) {
        children.set(0, parents.get(0));
        children.set(1, parents.get(1));
        return;
    }

    int tamClientes = clientesPadre1.size();
 
    int punto1 = rnd->nextInt(tamClientes - 1);
    int punto2 = rnd->nextInt( tamClientes - 1);

    if (punto1 > punto2) std::swap(punto1, punto2);

    std::vector<int> hijoClientes1(tamClientes, -1);
    std::vector<int> hijoClientes2(tamClientes, -1);
 
    std::vector<bool> usadosHijo1(max_node_id + 1, false);
    std::vector<bool> usadosHijo2(max_node_id + 1, false);

   
    for (int i = punto1; i <= punto2; i++) {
        hijoClientes1[i] = clientesPadre1[i];
        usadosHijo1[clientesPadre1[i]] = true;

        hijoClientes2[i] = clientesPadre2[i];
        usadosHijo2[clientesPadre2[i]] = true;
    }

 
    auto rellenar = [&](std::vector<int>& hijo, const std::vector<int>& padre, std::vector<bool>& usados) {
        int posHijo = (punto2 + 1) % tamClientes;
        for (int i = 0; i < tamClientes; ++i) {
            int posPadre = (punto2 + 1 + i) % tamClientes;
            int valor = padre[posPadre];

            if (!usados[valor]) {
                hijo[posHijo] = valor;
                posHijo = (posHijo + 1) % tamClientes;
            }
        }
        };

    rellenar(hijoClientes1, clientesPadre2, usadosHijo1);
    rellenar(hijoClientes2, clientesPadre1, usadosHijo2);

    // 3. Reconstruir las soluciones completas
    int indiceCliente1 = 0;
    int indiceCliente2 = 0;

    // Usamos la estructura original de cada padre para reconstruir cada hijo
    for (int i = 0; i < n; ++i) {
        // Hijo 1 reconstruido con plantilla de Padre 1
        int nodoOriginalP1 = parents.get(0).getVariableValue(i).L;
        if (nodoOriginalP1 != 0 && nodoOriginalP1 != -1) {
            children.get(0).setVariableValue(i, hijoClientes1[indiceCliente1++]);
        }
        else {
            children.get(0).setVariableValue(i, nodoOriginalP1);
        }

        // Hijo 2 reconstruido con plantilla de Padre 2
        int nodoOriginalP2 = parents.get(1).getVariableValue(i).L;
        if (nodoOriginalP2 != 0 && nodoOriginalP2 != -1) {
            children.get(1).setVariableValue(i, hijoClientes2[indiceCliente2++]);
        }
        else {
            children.get(1).setVariableValue(i, nodoOriginalP2);
        }
    }
}