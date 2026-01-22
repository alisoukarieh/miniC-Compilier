# Rapport de Projet - Compilateur MiniC

**Sorbonne Universite - Ecole Polytech' Sorbonne**
**Projet Compilation - Annee 2025-2026**

**Auteurs:** Emmanuel Pons, Ali Soukarieh

---

## Table des matieres

1. [Introduction](#1-introduction)
2. [Analyse Lexicale (lexico.l)](#2-analyse-lexicale-lexicol)
3. [Analyse Syntaxique (grammar.y)](#3-analyse-syntaxique-grammary)
4. [Fonctions Utilitaires (common.c)](#4-fonctions-utilitaires-commonc)
5. [Passe 1 - Verifications Contextuelles](#5-passe-1---verifications-contextuelles)
6. [Passe 2 - Generation de Code MIPS](#6-passe-2---generation-de-code-mips)
7. [Tests Realises](#7-tests-realises)
8. [Conclusion](#8-conclusion)

---

## 1. Introduction

Ce projet consiste a developper un compilateur pour le langage MiniC, un sous-ensemble du langage C. Le compilateur transforme un programme source MiniC en code assembleur MIPS. Ce rapport documente notre demarche de developpement, les choix de conception realises, et l'etat actuel du projet.

Notre approche de developpement a suivi l'ordre naturel du processus de compilation :
1. Analyse lexicale (tokenisation)
2. Analyse syntaxique (construction de l'AST)
3. Analyse semantique (verifications contextuelles - Passe 1)
4. Generation de code (Passe 2)

---

## 2. Analyse Lexicale (lexico.l)

### 2.1 Demarche de developpement

La premiere etape du projet consistait a implementer l'analyseur lexical dans le fichier `lexico.l`. Notre demarche a ete la suivante :

**Etape 1 : Definition des expressions regulieres**

Nous avons commence par definir les macros Flex pour les elements de base du langage :

```lex
LETTRE          [a-zA-Z]
CHIFFRE         [0-9]
CHIFFRE_NON_NUL [1-9]
IDF             {LETTRE}({LETTRE}|{CHIFFRE}|_)*
ENTIER_DEC      0|{CHIFFRE_NON_NUL}{CHIFFRE}*
LETTRE_HEXA     [a-fA-F]
ENTIER_HEXA     0x({CHIFFRE}|{LETTRE_HEXA})+
ENTIER          {ENTIER_DEC}|{ENTIER_HEXA}
```

**Raisonnement :** Nous avons suivi strictement la specification lexicographique du document de specifications (section 3). La distinction entre `CHIFFRE_NON_NUL` et `CHIFFRE` permet d'eviter les entiers decimaux commencant par 0 (qui seraient ambigus avec la notation octale en C standard).

**Etape 2 : Gestion des chaines de caracteres**

```lex
CHAINE_CAR     [^"\\\n]
CHAINE         \"({CHAINE_CAR}|\\\"|\\n)*\"
```

**Raisonnement :** La specification indique que les chaines peuvent contenir :
- Des caracteres imprimables sauf `"` et `\`
- Les sequences d'echappement `\"` et `\n`

Nous avons utilise une classe de caracteres negee `[^"\\\n]` pour exclure les caracteres problematiques.

**Etape 3 : Gestion des commentaires**

```lex
COMMENTAIRE     "//"[^\n]*\n
```

Les commentaires de style C++ (`//`) sont ignores jusqu'a la fin de ligne.

### 2.2 Decisions de conception importantes

**Gestion des debordements d'entiers :**

```c
val = strtol(yytext, &endptr, 0);
if (val < -2147483648 || val > 2147483647) {
    fprintf(stderr, "Error line %d: integer overflow for %s\n", yylineno, yytext);
    exit(1);
}
```

**Raisonnement :** Conformement a la specification (section 6.5), les entiers sont codes sur 32 bits. Nous detectons les debordements des l'analyse lexicale car la conversion caracteres -> entier se fait a ce moment.

**Mode debug pour les tokens :**

```c
#if LEX_DEBUG
#define RETURN(token) ({ printf("%s \t\"%s\"\n", #token, yytext); return token; })
#else
#define RETURN(token) ({ return token; })
#endif
```

**Raisonnement :** Cette macro conditionnelle permet de basculer facilement entre un mode debug (affichage des tokens) et le mode normal. Cela nous a permis de verifier que les tokens etaient correctement reconnus.

### 2.3 Verification

Pour verifier le bon fonctionnement de l'analyseur lexical, nous avons utilise le fichier de test `test/Syntaxe/OK/test_lex.c`. En mode `LEX_DEBUG`, le lexer affiche la liste des tokens reconnus, ce qui nous a permis de comparer avec les resultats attendus du TD.

---

## 3. Analyse Syntaxique (grammar.y)

### 3.1 Demarche de developpement

L'analyseur syntaxique construit l'arbre de syntaxe abstraite (AST) du programme. Notre demarche a suivi les etapes suivantes :

**Etape 1 : Declaration des tokens et des types**

```yacc
%token TOK_VOID TOK_INT TOK_BOOL TOK_TRUE TOK_FALSE TOK_IF TOK_DO TOK_WHILE TOK_FOR
%token TOK_PRINT TOK_SEMICOL TOK_COMMA TOK_LPAR TOK_RPAR TOK_LACC TOK_RACC

%token <intval> TOK_INTVAL;
%token <strval> TOK_IDENT TOK_STRING;

%type <ptr> program listdecl listdeclnonnull vardecl ident type listtypedecl decl maindecl
%type <ptr> listinst listinstnonnull inst block expr listparamprint paramprint
```

**Etape 2 : Definition des precedences d'operateurs**

```yacc
%nonassoc TOK_THEN
%nonassoc TOK_ELSE
%right TOK_AFFECT
%left TOK_OR
%left TOK_AND
...
%nonassoc TOK_UMINUS TOK_NOT TOK_BNOT
```

**Raisonnement :** L'ordre des declarations de precedence suit exactement la specification (section 4.1). Le conflit classique "dangling else" est resolu par les pseudo-tokens `TOK_THEN` et `TOK_ELSE`. Le moins unaire (`TOK_UMINUS`) a une precedence plus elevee que le moins binaire.

**Etape 3 : Implementation des regles de grammaire**

Nous avons implemente toutes les regles de la grammaire hors-contexte specifiee dans la section 4.2 du document. Pour chaque regle, nous construisons le noeud AST correspondant.

### 3.2 Fonctions de construction de l'arbre

**Fonction generique `make_node` :**

```c
node_t make_node(node_nature nature, int nops, ...) {
    node_t n = malloc(sizeof(node_s));
    n->nature = nature;
    n->nops = nops;
    n->lineno = yylineno;

    if (nops > 0) {
        n->opr = malloc(nops * sizeof(node_t));
        va_list ap;
        va_start(ap, nops);
        for (int i = 0; i < nops; i++)
            n->opr[i] = va_arg(ap, node_t);
        va_end(ap);
    }
    return n;
}
```

**Raisonnement :** L'utilisation de `va_list` permet de creer des noeuds avec un nombre variable d'enfants de maniere elegante. Le numero de ligne (`yylineno`) est capture a la creation pour permettre des messages d'erreur precis.

**Fonctions specialisees :**

- `make_type(node_type t)` : Cree un noeud NODE_TYPE avec le type specifie
- `make_ident(const char* s)` : Cree un noeud NODE_IDENT avec le nom de l'identificateur
- `make_int(int64_t v)` : Cree un noeud NODE_INTVAL avec la valeur entiere
- `make_bool(bool b)` : Cree un noeud NODE_BOOLVAL avec la valeur booleenne
- `make_string(const char* s)` : Cree un noeud NODE_STRINGVAL avec la chaine

**Fonction `list_append` :**

```c
node_t list_append(node_t list, node_t elem) {
    return make_node(NODE_LIST, 2, list, elem);
}
```

**Raisonnement :** Les listes dans l'AST sont representees par des noeuds NODE_LIST imbriques. Cette representation recursive correspond exactement a la grammaire d'arbres specifiee (regles 0.4, 0.7, 0.16, etc.).

### 3.3 Verification

Pour verifier la construction de l'AST, nous avons utilise :
1. Le fichier `test/Syntaxe/OK/test_yacc.c` (meme code que le TD)
2. La fonction `dump_tree()` qui genere un fichier DOT visualisable avec xdot

L'arbre genere correspond a celui presente dans la Figure 2 de la specification, validant ainsi notre implementation.

---

## 4. Fonctions Utilitaires (common.c)

### 4.1 Analyse des arguments de la ligne de commande

**Implementation de `parse_args` :**

```c
void parse_args(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "bo:t:r:svh")) != -1) {
        switch (opt) {
            case 'b': banner = true; break;
            case 'o': outfile = optarg; break;
            case 't': trace_level = atoi(optarg); break;
            case 'r': max_reg = atoi(optarg); break;
            case 's': stop_after_syntax = true; break;
            case 'v': stop_after_verif = true; break;
            case 'h': help = true; break;
        }
    }
    // Validations...
}
```

**Raisonnement :** Nous avons utilise `getopt()` malgre la recommandation de la specification car elle simplifie grandement le parsing des options. Les validations suivent exactement les regles de la section 8.1 :
- `-b` doit etre utilise seul
- `-s` et `-v` sont mutuellement exclusifs
- Un seul fichier d'entree est autorise
- Les valeurs de `-t` et `-r` sont validees

### 4.2 Gestion de la memoire - `free_nodes`

```c
void free_nodes(node_t n) {
    if (n == NULL) return;

    // Liberation recursive des enfants
    for (int i = 0; i < n->nops; i++) {
        free_nodes(n->opr[i]);
    }

    // Liberation du tableau d'operandes
    if (n->opr != NULL) free(n->opr);

    // Liberation des chaines allouees
    if (n->ident != NULL) free(n->ident);
    if (n->str != NULL) free(n->str);

    // Liberation du noeud lui-meme
    free(n);
}
```

**Raisonnement :** On libere d'abord les enfants, puis le noeud courant. Les champs `ident` et `str` sont des chaines allouees dynamiquement qui doivent etre liberees separement.

**Verification avec Valgrind :**

```bash
valgrind --leak-check=full ./minicc test.c
```

Les tests Valgrind confirment l'absence de fuites memoire pour les programmes corrects.

### 4.3 Affichage de l'arbre - `dump_tree`

Cette fonction genere une representation DOT de l'AST, permettant la visualisation avec xdot. Elle a ete tres utile pour le debug, notamment pour verifier que l'arbre construit correspond bien a la grammaire d'arbres.

---

## 5. Passe 1 - Verifications Contextuelles

### 5.1 Architecture de la passe 1

La passe 1 effectue les verifications semantiques et decore l'arbre. Notre implementation dans `passe_1.c` suit la structure suivante :

```c
void analyse_passe_1(node_t root) {
    node_t globals = root->opr[0];
    node_t mainf   = root->opr[1];

    // Traitement des declarations globales
    push_global_context();
    decls_list(globals, true);

    // Traitement de la fonction main
    main_decl(mainf);

    pop_context();
}
```

### 5.2 Gestion des declarations

**Fonction `decls_list` :**

Parcourt recursivement les listes de declarations (NODE_LIST ou NODE_DECLS).

**Fonction `decl_list` :**

```c
void decl_list(node_t decl, node_type type, bool is_global) {
    // ...
    offset = env_add_element(ident_node->ident, ident_node);

    if (type == TYPE_VOID)
        error_rule(ident_node, "1.8", "Variable '%s' cannot be of type void", ident_node->ident);

    if (offset == -1)
        error_rule(ident_node, "1.11", "Variable '%s' already declared", ident_node->ident);

    // Verification de l'initialisation pour les variables globales
    if (decl->nops == 2 && is_global) {
        if (!(decl->opr[1]->nature == NODE_INTVAL || decl->opr[1]->nature == NODE_BOOLVAL))
            error_rule(..., "1.12", "Expression are not allowed...");
        if (decl->opr[1]->type != type)
            error_rule(..., "1.12", "Type mismatch...");
    }
    // ...
}
```

**Verifications implementees :**
- Regle 1.8 : Le type `void` ne peut pas etre utilise pour les variables
- Regle 1.11 : Une variable ne peut pas etre declaree deux fois dans le meme contexte
- Regle 1.12 : Les variables globales ne peuvent etre initialisees qu'avec des litteraux
- Regle 1.12 : Le type de l'expression d'initialisation doit correspondre au type declare

### 5.3 Verification de la fonction main

```c
void main_decl(node_t func_node) {
    reset_env_current_offset();

    node_t type = func_node->opr[0];
    node_t name = func_node->opr[1];
    node_t block = func_node->opr[2];

    // Regle 1.5 : nom doit etre "main"
    if (strcmp(name->ident, "main") != 0)
        error_rule(name, "1.4", "The main function must be named 'main'");

    // Regle 1.5 : type de retour doit etre void
    if (type->type != TYPE_VOID)
        error_rule(type, "1.4", "The main function must return 'void'");

    block_decl(block);
    func_node->offset = get_env_current_offset();
}
```

### 5.4 Gestion de l'environnement

Nous utilisons les fonctions fournies par la bibliotheque `miniccutils` :
- `push_global_context()` : Cree le contexte global
- `push_context()` : Empile un nouveau contexte (pour les blocs)
- `pop_context()` : Depile le contexte courant
- `env_add_element()` : Ajoute une variable au contexte courant
- `get_decl_node()` : Recherche une variable dans l'environnement
- `get_env_current_offset()` : Recupere l'offset courant pour l'allocation en pile

### 5.5 Traitement des expressions

La fonction `expr_processing` effectue la verification de types et la decoration de l'arbre pour toutes les expressions. L'implementation utilise un `switch` sur la nature du noeud :

**Operateurs arithmetiques (PLUS, MINUS, MUL, DIV, MOD) :**

```c
case NODE_PLUS:
case NODE_MINUS:
case NODE_MUL:
case NODE_DIV:
case NODE_MOD:
    expr_processing(left);
    expr_processing(right);
    if (left->type != TYPE_INT || right->type != TYPE_INT) {
        error_rule(expr, "1.30", "Operands of arithmetic operators must be of type int");
    }
    expr->type = TYPE_INT;
    break;
```

**Raisonnement :** Les operateurs arithmetiques necessitent des operandes de type `int` et produisent un resultat de type `int` (regle 1.30).

**Operateurs de comparaison (LT, GT, LE, GE) :**

Ces operateurs prennent des operandes `int` et retournent un `bool` (regle 1.30).

**Operateurs d'egalite (EQ, NE) :**

```c
case NODE_EQ:
case NODE_NE:
    expr_processing(left);
    expr_processing(right);
    if (left->type != right->type) {
        error_rule(expr, "1.30", "Operands of equality operators must have the same type");
    }
    expr->type = TYPE_BOOL;
    break;
```

**Raisonnement :** Les operateurs d'egalite acceptent des operandes de meme type (int ou bool) et retournent un `bool`.

**Operateurs logiques (AND, OR) :**

Necessitent des operandes `bool` et retournent un `bool` (regle 1.30).

**Operateurs bit-a-bit (BAND, BOR, BXOR, SLL, SRA, SRL) :**

Necessitent des operandes `int` et retournent un `int` (regle 1.30).

**Operateurs unaires :**

```c
case NODE_NOT:
    expr_processing(left);
    if (left->type != TYPE_BOOL) {
        error_rule(expr, "1.31", "Operand of logical not operator must be of type bool");
    }
    expr->type = TYPE_BOOL;
    break;

case NODE_UMINUS:
case NODE_BNOT:
    expr_processing(left);
    if (left->type != TYPE_INT) {
        error_rule(expr, "1.31", "Operand must be of type int");
    }
    expr->type = TYPE_INT;
    break;
```

**Resolution des identificateurs (NODE_IDENT) :**

```c
case NODE_IDENT:{
    node_t ident_node = get_decl_node(expr->ident);
    if (ident_node == NULL) {
        error_rule(expr, "1.61", "Variable '%s' not declared", expr->ident);
    }
    expr->type = ident_node->type;
    expr->offset = ident_node->offset;
    expr->decl_node = ident_node;
    expr->global_decl = ident_node->global_decl;
    break;
}
```

**Raisonnement :** Pour chaque utilisation d'un identificateur, on recherche sa declaration dans l'environnement avec `get_decl_node()`. Si la variable n'existe pas, on signale une erreur (regle 1.61). Sinon, on decore le noeud avec les informations de la declaration : type, offset, pointeur vers le noeud de declaration, et indicateur global.

**Affectation (NODE_AFFECT) :**

```c
case NODE_AFFECT:
    expr_processing(left);
    expr_processing(right);
    if (left->nature != NODE_IDENT) {
        error_rule(expr, "1.32", "Left operand of assignment must be a variable");
    }
    if (left->type != right->type) {
        error_rule(expr, "1.32", "Type mismatch in assignment to variable '%s'", left->ident);
    }
    expr->type = left->type;
    break;
```

**Raisonnement :** L'operande gauche doit etre un identificateur (lvalue) et les types doivent correspondre (regle 1.32).

### 5.6 Traitement des instructions

La fonction `instr_processing` verifie les instructions de controle :

**Instruction IF :**

```c
case NODE_IF: {
    expr_processing(left);
    if (left->type != TYPE_BOOL) {
        error_rule(left, "1.18", "Expression in if statement must be of type bool");
    }
    instr_processing(right);
    if (else_instr) instr_processing(else_instr);
    break;
}
```

**Raisonnement :** La condition d'un `if` doit etre de type `bool` (regle 1.18). Les branches then et else sont traitees recursivement.

**Instruction WHILE :**

La condition doit etre de type `bool` (regle 1.20).

**Instruction FOR :**

```c
case NODE_FOR:{
    expr_processing(left);      // initialisation
    expr_processing(cond);      // condition
    expr_processing(increment); // increment
    if (cond->type != TYPE_BOOL) {
        error_rule(cond, "1.21", "Condition expression in for statement must be of type bool");
    }
    instr_processing(right);    // corps
    break;
}
```

**Raisonnement :** Seule l'expression de condition doit etre de type `bool` (regle 1.21). Les expressions d'initialisation et d'increment peuvent etre de n'importe quel type.

**Instruction DO-WHILE :**

La condition doit etre de type `bool` (regle 1.22).

**Blocs imbriques :**

```c
case NODE_BLOCK:
    block_decl(instr);
    break;
```

Les blocs imbriques sont traites recursivement avec `block_decl`, ce qui cree un nouveau contexte pour les variables locales.

### 5.7 Traitement de l'instruction PRINT

```c
void print_processing(node_t print_node){
    if (print_node == NULL) return;

    if (print_node->nature == NODE_IDENT){
        node_t ident_node = get_decl_node(print_node->ident);
        if (ident_node == NULL) {
            error_rule(print_node, "1.61", "Variable '%s' not declared", print_node->ident);
        }
        print_node->type = ident_node->type;
        print_node->offset = ident_node->offset;
        print_node->decl_node = ident_node;
        print_node->global_decl = ident_node->global_decl;
    }
}
```

**Raisonnement :** Les parametres de `print` peuvent etre des chaines ou des identificateurs. Pour les identificateurs, on verifie qu'ils sont declares et on decore le noeud comme pour les autres utilisations de variables.

---

## 6. Passe 2 - Generation de Code MIPS

### 6.1 Architecture de la passe 2

La passe 2 genere le code assembleur MIPS a partir de l'AST decore. Notre implementation dans `passe_2.c` suit la structure suivante :

```c
void gen_code_passe_2(node_t root) {
    collect_strings(root);

    set_max_registers(get_num_registers());
    reset_temporary_max_offset();

    gen_data_section(root);

    if (root->opr[1] != NULL) {
        gen_text_section(root->opr[1]);
    }
}
```

### 6.2 Probleme initial : allocation de registres

Notre premiere implementation de la generation de code utilisait une approche naive pour l'allocation de registres. Nous appelions `allocate_reg()` dans les noeuds feuilles (NODE_INTVAL, NODE_BOOLVAL, NODE_IDENT), ce qui produisait des numeros de registres decales par rapport au compilateur de reference.

**Exemple du probleme :**
```
Notre sortie:     ori $9, $0, 42
Reference:        ori $8, $0, 42
```

Apres analyse, nous avons decouvert que la fonction `get_current_reg()` de la bibliotheque retourne le registre "courant" (initialement $8), et que `allocate_reg()` avance le compteur vers le registre suivant.

### 6.3 Solution : pattern correct d'allocation

La solution est decrite dans le PDF de specification. Le pattern correct est :

**Pour les noeuds feuilles :**
- Utiliser directement `get_current_reg()` pour obtenir le registre
- Ne PAS appeler `allocate_reg()` - c'est l'appelant qui gere l'allocation

**Pour les operations binaires :**
```c
gen_expr(expr->opr[0]);           // Resultat dans get_current_reg()
reg_left = get_current_reg();     // Sauvegarder le registre
spilled = !reg_available();       // Verifier si spilling necessaire
if (spilled) {
    push_temporary(reg_left);     // Sauvegarder sur la pile
}
allocate_reg();                   // Reserver un registre pour l'operande droit
gen_expr(expr->opr[1]);           // Resultat dans get_current_reg()
reg_right = get_current_reg();
// ... effectuer l'operation ...
release_reg();                    // Liberer le registre de l'operande droit
```

Apres avoir applique ce pattern, notre sortie correspond exactement a celle du compilateur de reference.

### 6.4 Generation de la section .data

```c
static void gen_data_section(node_t root) {
    create_data_sec_inst();

    if (root->opr[0] != NULL) {
        gen_global_decls(root->opr[0]);
    }

    int32_t num_strings = get_global_strings_number();
    for (int32_t i = 0; i < num_strings; i++) {
        create_asciiz_inst(NULL, get_global_string(i));
    }
}
```

La section .data contient :
- Les variables globales (avec leur valeur initiale ou 0)
- Les chaines de caracteres collectees dans l'arbre

### 6.5 Generation des expressions

Chaque type d'expression est traite dans un switch :

**Litteraux entiers et booleens :**
```c
case NODE_INTVAL:
case NODE_BOOLVAL:
    reg = get_current_reg();
    if (expr->value >= 0 && expr->value <= 0xFFFF) {
        create_ori_inst(reg, get_r0(), (int32_t)(expr->value & 0xFFFF));
    } else {
        create_lui_inst(reg, (int32_t)((expr->value >> 16) & 0xFFFF));
        create_ori_inst(reg, reg, (int32_t)(expr->value & 0xFFFF));
    }
    break;
```

**Chargement de variables :**
```c
case NODE_IDENT:
    if (decl != NULL && decl->global_decl) {
        create_lui_inst(reg, 0x1001);
        create_lw_inst(reg, decl->offset, reg);
    } else {
        create_lw_inst(reg, expr->offset, get_stack_reg());
    }
    break;
```

### 6.6 Gestion du spilling de registres

Quand tous les registres temporaires sont utilises, on sauvegarde sur la pile :

```c
case NODE_PLUS:
    gen_expr(expr->opr[0]);
    reg_left = get_current_reg();
    spilled = !reg_available();
    if (spilled) {
        push_temporary(reg_left);
    }
    allocate_reg();
    gen_expr(expr->opr[1]);
    reg_right = get_current_reg();
    if (spilled) {
        pop_temporary(get_restore_reg());
        create_addu_inst(reg_right, get_restore_reg(), reg_right);
    } else {
        create_addu_inst(reg_left, reg_left, reg_right);
        release_reg();
    }
    break;
```

### 6.7 Structures de controle

**Instruction IF :**
```c
static void gen_if(node_t node) {
    int32_t label_else = get_new_label();

    gen_expr(node->opr[0]);
    create_beq_inst(get_current_reg(), get_r0(), label_else);

    gen_instr(node->opr[1]);

    if (node->opr[2] != NULL) {
        int32_t label_end = get_new_label();
        create_j_inst(label_end);
        create_label_inst(label_else);
        gen_instr(node->opr[2]);
        create_label_inst(label_end);
    } else {
        create_label_inst(label_else);
    }
}
```

**Instruction WHILE :**
```c
static void gen_while(node_t node) {
    int32_t label_start = get_new_label();
    int32_t label_end = get_new_label();

    create_label_inst(label_start);
    gen_expr(node->opr[0]);
    create_beq_inst(get_current_reg(), get_r0(), label_end);
    gen_instr(node->opr[1]);
    create_j_inst(label_start);
    create_label_inst(label_end);
}
```

### 6.8 Gestion de la pile

Nous utilisons les fonctions de la bibliotheque pour gerer la pile :

```c
static void gen_text_section(node_t func) {
    create_text_sec_inst();
    create_label_str_inst("main");

    set_temporary_start_offset(func->offset);
    create_stack_allocation_inst();

    if (func->opr[2] != NULL) {
        gen_block(func->opr[2]);
    }

    int32_t stack_size = get_temporary_max_offset();
    if (func->offset > stack_size) {
        stack_size = func->offset;
    }
    create_stack_deallocation_inst(stack_size);
    create_ori_inst(2, get_r0(), 0xa);
    create_syscall_inst();
}
```

### 6.9 Division par zero

Pour les operations DIV et MOD, nous ajoutons une verification :
```c
create_div_inst(reg_left, reg_right);
create_teq_inst(reg_right, get_r0());  // Trap si diviseur = 0
create_mflo_inst(reg_left);            // ou mfhi pour MOD
```

### 6.10 Resultats des tests

| Categorie | Resultat |
|-----------|----------|
| Tests OK (generation) | 14/14 pass |
| Tests KO (division par zero) | 3/3 pass |
| **Total** | **17/17 pass** |

---

## 7. Tests Realises

### 7.1 Tests syntaxiques OK

- `test_lex.c` : Verification des tokens lexicaux
- `test_yacc.c` : Verification de la construction de l'AST (code du TD)

### 7.2 Tests de verification KO (Tests/Verif/KO/)

- `test_passe1_111.c` : Variable deja declaree (regle 1.11)
- `test_passe1_113.c` : Variable non declaree (regle 1.61)
- `test_passe1_12_exp.c` : Expression dans initialisation globale (regle 1.12)
- `test_passe1_14.c` : Nom de fonction != "main" (regle 1.5)
- `test_passe1_14_int.c` : Type de retour de main != void (regle 1.5)
- `test_passe1_void.c` : Variable de type void (regle 1.8)

### 7.3 Regles de typage verifiees

La passe 1 verifie maintenant l'ensemble des regles de typage :

| Regle | Description | Implementation |
|-------|-------------|----------------|
| 1.8   | Variables ne peuvent pas etre de type void | `decl_list()` |
| 1.11  | Double declaration interdite | `decl_list()` |
| 1.12  | Initialisation globale = litteraux uniquement | `decl_list()` |
| 1.13  | Compatibilite type initialisation | `decl_list()` |
| 1.18  | Condition if doit etre bool | `instr_processing()` |
| 1.20  | Condition while doit etre bool | `instr_processing()` |
| 1.21  | Condition for doit etre bool | `instr_processing()` |
| 1.22  | Condition do-while doit etre bool | `instr_processing()` |
| 1.30  | Types operateurs binaires | `expr_processing()` |
| 1.31  | Types operateurs unaires | `expr_processing()` |
| 1.32  | Affectation : lvalue + compatibilite | `expr_processing()` |
| 1.61  | Variable doit etre declaree | `expr_processing()` |

---

## 8. Conclusion

### 8.1 Fonctionnalites implementees

| Composant               | Etat    | Description                                           |
| ----------------------- | ------- | ----------------------------------------------------- |
| Analyse lexicale        | Complet | Tous les tokens reconnus                              |
| Analyse syntaxique      | Complet | AST conforme a la grammaire                           |
| Arguments CLI           | Complet | Toutes les options supportees                         |
| Liberation memoire      | Complet | Pas de fuites memoire                                 |
| Passe 1 - Declarations  | Complet | Declarations globales et locales                      |
| Passe 1 - Fonction main | Complet | Verifications nom et type                             |
| Passe 1 - Expressions   | Complet | Verification de types, resolution des identificateurs |
| Passe 1 - Instructions  | Complet | Verification des conditions booleennes                |
| Passe 1 - Print         | Complet | Verification des parametres de print                  |
| Passe 2 - Generation    | Complet | Generation de code MIPS                               |

### 8.2 Bilan du projet

Ce projet nous a permis d'apprehender concretement les differentes etapes de la compilation. La demarche incrementale - d'abord le lexer, puis le parser, puis les verifications semantiques, et enfin la generation de code - s'est averee efficace pour deboguer progressivement chaque composant.

**Passe 1 :** Toutes les verifications contextuelles sont implementees (declarations, types des expressions, conditions des structures de controle) et l'arbre est entierement decore avec les informations necessaires pour la generation de code.

**Passe 2 :** La generation de code MIPS est complete. Une difficulte importante a ete l'allocation des registres : notre premiere implementation produisait des numeros de registres decales par rapport au compilateur de reference ($9 au lieu de $8). Apres avoir etudie le fonctionnement des fonctions de la bibliotheque (`get_current_reg()`, `allocate_reg()`, `release_reg()`), nous avons compris que les noeuds feuilles ne doivent pas appeler `allocate_reg()` - c'est l'appelant qui gere l'allocation. En appliquant le pattern decrit dans le PDF, notre sortie correspond maintenant exactement a celle du compilateur de reference.

L'utilisation des outils de visualisation (xdot pour les arbres, Valgrind pour la memoire) et la comparaison avec le compilateur de reference ont ete precieuses pour valider notre implementation.

### 8.3 Resultats finaux

Le compilateur MiniC est maintenant fonctionnel et passe tous les tests :
- **Tests de verification (Passe 1)** : Tous les tests OK et KO passent
- **Tests de generation (Passe 2)** : 17/17 tests passent avec sortie identique a la reference
