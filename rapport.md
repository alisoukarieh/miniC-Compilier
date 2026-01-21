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
6. [Tests Realises](#6-tests-realises)
7. [Etat d'Avancement et Travaux Futurs](#7-etat-davancement-et-travaux-futurs)

---

## 1. Introduction

Ce projet consiste a developper un compilateur pour le langage MiniC, un sous-ensemble du langage C. Le compilateur transforme un programme source MiniC en code assembleur MIPS. Ce rapport documente notre demarche de developpement, les choix de conception realises, et l'etat actuel du projet.

Notre approche de developpement a suivi l'ordre naturel du processus de compilation :
1. Analyse lexicale (tokenisation)
2. Analyse syntaxique (construction de l'AST)
3. Analyse semantique (verifications contextuelles - Passe 1)
4. Generation de code (Passe 2 - a completer)

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

**Raisonnement :** Une gestion correcte de la memoire est essentielle. Nous avons adopte une approche de parcours en profondeur post-ordre : on libere d'abord les enfants, puis le noeud courant. Les champs `ident` et `str` sont des chaines allouees dynamiquement qui doivent etre liberees separement.

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
- `get_env_current_offset()` : Recupere l'offset courant pour l'allocation en pile

---

## 6. Tests Realises

### 6.1 Tests syntaxiques OK

- `test_lex.c` : Verification des tokens lexicaux
- `test_yacc.c` : Verification de la construction de l'AST (code du TD)

### 6.2 Tests syntaxiques KO

- `test_passe1_111.c` : Variable deja declaree (regle 1.11)
- `test_passe1_113.c` : Variable non declaree (regle 1.13 - a implementer)
- `test_passe1_12_exp.c` : Expression dans initialisation globale (regle 1.12)
- `test_passe1_14.c` : Nom de fonction != "main" (regle 1.5)
- `test_passe1_14_int.c` : Type de retour de main != void (regle 1.5)
- `test_passe1_void.c` : Variable de type void (regle 1.8)

---

## 7. Etat d'Avancement et Travaux Futurs

### 7.1 Fonctionnalites implementees

| Composant | Etat | Description |
|-----------|------|-------------|
| Analyse lexicale | Complet | Tous les tokens reconnus |
| Analyse syntaxique | Complet | AST conforme a la grammaire |
| Arguments CLI | Complet | Toutes les options supportees |
| Liberation memoire | Complet | Pas de fuites memoire |
| Passe 1 - Declarations | Partiel | Declarations globales et locales |
| Passe 1 - Fonction main | Complet | Verifications nom et type |
| Passe 1 - Expressions | A faire | Verification de types, resolution des identificateurs |
| Passe 1 - Instructions | A faire | Verification des conditions booleennes |
| Passe 2 | A faire | Generation de code MIPS |

### 7.2 Travaux restants pour la Passe 1

1. **Traitement des expressions** :
   - Verification des types pour les operateurs
   - Resolution des identificateurs (liaison avec `decl_node`)
   - Verification de compatibilite pour les affectations

2. **Traitement des instructions** :
   - Verification que les conditions (if, while, for, do-while) sont de type boolean
   - Traitement recursif des blocs imbriques

3. **Decoration de l'arbre** :
   - Mise a jour du champ `type` pour les noeuds d'expression
   - Mise a jour du champ `decl_node` pour les NODE_IDENT d'utilisation

### 7.3 Travaux pour la Passe 2

1. Generation de la section `.data` (variables globales, chaines)
2. Generation du prologue de la fonction main
3. Generation du code pour les expressions
4. Generation du code pour les structures de controle
5. Generation de l'epilogue et appel systeme exit

---

## Conclusion

Ce projet nous a permis d'apprehender concretement les differentes etapes de la compilation. La demarche incrementale - d'abord le lexer, puis le parser, puis les verifications semantiques - s'est averee efficace pour deboguer progressivement chaque composant. L'utilisation des outils de visualisation (xdot pour les arbres, Valgrind pour la memoire) a ete precieuse pour valider notre implementation.

Les choix de conception ont ete guides par la specification et par un souci de clarte du code. La passe 1 est partiellement implementee et la passe 2 reste a developper.
