/* $Id: fndeclstate.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ -*-c-*-**/

/*
  This file is part of Ctalk.
  Copyright © 2018 Robert Kiesling, rk3314042@gmail.com.
  Permission is granted to copy this software provided that this copyright
  notice is included in all source code modules.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 *  Function declaration states.  is_c_fn_declaration_msg (cparse.c)
 *  depends on this order.
 */

  ARGSEPARATOR,	LABEL,		/* 0. */
  LABEL,	ARGSEPARATOR,	/* 1. */

  LABEL,	LABEL,		/* 2. Return val declaration w/o lookahed for a C type. */
  LABEL,  	ASTERISK,	/* 3. Return val pointer declaration. */
  LABEL,	OPENPAREN,	/* 4. Start of param list. */
  OPENPAREN,	ASTERISK,	/* 5. The declaration is a function ptr. */
  OPENPAREN,	LABEL,		/* 6. Another decl type for func ptrs.   */
  LABEL,	CLOSEPAREN,	/* 7.  ... both followed by this. */
  CLOSEPAREN,	OPENPAREN,	/* 8. Start of param list of ptr decl. */
  CLOSEPAREN,	SEMICOLON,	/* 9. End of prototype decl. */ 
  CLOSEPAREN,	OPENBLOCK,	/* 10. End of actual decl.  */
  ASTERISK,	LABEL,		/* 11. */
  OPENPAREN,	CLOSEPAREN,	/* 12. */
  OPENPAREN,	ELLIPSIS,	/* 13. */
  ELLIPSIS,	CLOSEPAREN,	/* 14. */

  ASTERISK,	ASTERISK,	/* 15. */
  ASTERISK,	ARGSEPARATOR,	/* 16. Prototype: e.g., "... char *, ..." */
  ASTERISK,	OPENPAREN,	/* 17. fn arg: ... *( .. eg: *(*)*/
  ASTERISK,	CLOSEPAREN,	/* 18. fn arg: ... *) */
  CLOSEPAREN,	ARGSEPARATOR,	/* 19. */
  CLOSEPAREN,	CLOSEPAREN,	/* 20. */
  OPENPAREN,	OPENPAREN,	/* 21. */
  ARGSEPARATOR,	ELLIPSIS,	/* 22. */
  ELLIPSIS,	CLOSEPAREN,	/* 23. */

#ifdef __GNUC__
  CLOSEPAREN,	LABEL,		/* 24   ) __attribute__ ((n)); */
#endif

  OPENPAREN,	LABEL,		/* 25. */
  LABEL,	CHAR,		/* 26. */
  LABEL,	ASTERISK,	/* 27. */
  LABEL,	LABEL,		/* 28. */
  LABEL,	CLOSEPAREN,	/* 29. */
  LABEL,	ARGSEPARATOR,	/* 30. */
  ARGSEPARATOR,	LABEL,		/* 31. */
  CHAR,		LABEL,		/* 32. */
  ASTERISK,	LABEL,		/* 33. */
  ASTERISK,	ASTERISK,	/* 34. */
  CLOSEPAREN,	OPENBLOCK,	/* 35. */
  OPENPAREN,	ELLIPSIS,	/* 36. */
  ELLIPSIS,	CLOSEPAREN,	/* 37. */
  CLOSEPAREN,	SEMICOLON,	/* 38. */
  CLOSEPAREN,	OPENBLOCK,	/* 39. */
  LABEL,	OPENPAREN,	/* 40. */
  OPENPAREN,	LITERAL,	/* 41. */
  LITERAL,	CLOSEPAREN,	/* 42. */
  CLOSEPAREN,	CLOSEPAREN,	/* 43. */
  ARGSEPARATOR,	ELLIPSIS,	/* 44. */
  ELLIPSIS,	ARGSEPARATOR,	/* 45. */
  ELLIPSIS,	CLOSEPAREN,	/* 46. */
  ASTERISK,	OPENPAREN,	/* 47.  e.g. OBJECT *(*)() */
  OPENPAREN,	ASTERISK,	/* 48. ... */
  ASTERISK,	CLOSEPAREN,	/* 49. ...      */
  CLOSEPAREN,	OPENPAREN,	/* 50. ...      */
  OPENPAREN,	CLOSEPAREN,	/* 51. ...      */
  CLOSEPAREN,	ARGSEPARATOR,	/* 52. ...      */
  LABEL,	ARRAYOPEN,	/* 53. */
  ARRAYCLOSE,	ARGSEPARATOR,	/* 54. */
  ARRAYCLOSE,	CLOSEPAREN,	/* 55. */
  ARRAYOPEN,	CHAR,		/* 56. Subscript constant expressions. */
  ARRAYOPEN,	LABEL,		/* 57. ... */
  ARRAYOPEN,	INTEGER,	/* 58. ... */
  ARRAYOPEN,	FLOAT,		/* 59. ... */
  ARRAYOPEN,	DOUBLE,		/* 60. ... */
  ARRAYOPEN,	LONG,		/* 61. ... */
  ARRAYOPEN,	LONGLONG,	/* 62. ... */
  ARRAYOPEN,	AMPERSAND,	/* 63. ... */
  ARRAYOPEN,	EXCLAM,		/* 64. ... */
  ARRAYOPEN,	INCREMENT,	/* 65. ... */
  ARRAYOPEN,	DECREMENT,	/* 66. ... */
  ARRAYOPEN,	BIT_COMP,	/* 67. ... */
  ARRAYOPEN,	ASTERISK,	/* 68. ... */
  ARRAYOPEN,	ARRAYCLOSE,	/* 69. ... */
  /* 
   * The next two states are for parameter prototypes
   * that do not include the parameter name.
   * In these cases the prototype is not listed as
   * a parameter.
   */
  ASTERISK,   ARGSEPARATOR,  /* 70. */
  ASTERISK,   CLOSEPAREN,    /* 71. */
  OPENPAREN,  CHAR,          /* 72. */
  OPENPAREN,  INTEGER,       /* 73. */
  OPENPAREN,  FLOAT,         /* 74. */
  OPENPAREN,       DOUBLE,                /* 75. */
  OPENPAREN,       LONG,                  /* 76. */
  OPENPAREN,       LONGLONG,             /* 77. */
  OPENPAREN,       AMPERSAND,           /* 78. */
  OPENPAREN,       EXCLAM,              /* 79. */
  OPENPAREN,       INCREMENT,           /* 80. */
  OPENPAREN,       DECREMENT,           /* 81. */
  OPENPAREN,       BIT_COMP,            /* 82. */
  OPENPAREN,       ASTERISK,            /* 83. */
  /*
   *  More closing paren states.
   */
  CHAR,            CLOSEPAREN,          /* 84. */
  INTEGER,         CLOSEPAREN,          /* 85. */
  FLOAT,           CLOSEPAREN,          /* 86. */
  DOUBLE,          CLOSEPAREN,          /* 87. */
  LONG,            CLOSEPAREN,          /* 88. */
  LONGLONG,        CLOSEPAREN,          /* 89. */
  INCREMENT,       CLOSEPAREN,          /* 90. */
  DECREMENT,       CLOSEPAREN,          /* 91. */

  /*
   *  States for args that end at the argument separator.
   */
  CHAR,            ARGSEPARATOR,        /* 92. */
  INTEGER,         ARGSEPARATOR,        /* 93. */
  FLOAT,           ARGSEPARATOR,        /* 94. */
  DOUBLE,          ARGSEPARATOR,        /* 95. */
  LONG,            ARGSEPARATOR,        /* 96. */
  LONGLONG,        ARGSEPARATOR,        /* 97. */
  INCREMENT,       ARGSEPARATOR,        /* 98. */
  DECREMENT,       ARGSEPARATOR,        /* 99. */
  LITERAL,         ARGSEPARATOR,        /* 100. */
  LITERAL_CHAR,    ARGSEPARATOR,        /* 101. */

  LABEL,           DEREF,               /* 102. */
  DEREF,           LABEL,               /* 103. */

  ARGSEPARATOR,    LITERAL,             /* 104. */
  ARGSEPARATOR,    LITERAL_CHAR,        /* 105. */
  ARGSEPARATOR,    CHAR,                /* 106. */
  ARGSEPARATOR,    INTEGER,             /* 107. */
  ARGSEPARATOR,    FLOAT,               /* 108. */
  ARGSEPARATOR,    DOUBLE,              /* 109. */
  ARGSEPARATOR,    LONG,                /* 110. */
  ARGSEPARATOR,    LONGLONG,            /* 111. */
  ARGSEPARATOR,    AMPERSAND,           /* 112. */
  ARGSEPARATOR,    EXCLAM,              /* 113. */
  ARGSEPARATOR,    INCREMENT,           /* 114. */
  ARGSEPARATOR,    DECREMENT,           /* 115. */
  ARGSEPARATOR,    BIT_COMP,            /* 116. */
  ARGSEPARATOR,    ASTERISK,            /* 117. */

  AMPERSAND,       LABEL,               /* 118. */
  LABEL,           LABEL,               /* 119. */
  /*
   *   More unusual states.
   */
  LITERAL_CHAR,    CLOSEPAREN,          /* 120. */
  ARGSEPARATOR,    OPENPAREN,           /* 121. */
  OPENPAREN,       OPENPAREN,           /* 122. */
  CLOSEPAREN,      INTEGER,             /* 123. */
  ASTERISK,        CLOSEPAREN,          /* 124. */
  OPENPAREN,       LITERAL_CHAR,        /* 125. */
  CLOSEPAREN,      LABEL,               /* 126. */
  LABEL,           PERIOD,              /* 127. */
  PERIOD,          LABEL,               /* 128. */

  /*
   *  States for parameter expressions.
   */
  CLOSEPAREN,      DEREF,               /* 129. */
  CLOSEPAREN,      PLUS,                /* 130. */
  CLOSEPAREN,      MINUS,               /* 131. */
  CLOSEPAREN,      ASTERISK,            /* 132. */
  CLOSEPAREN,      DIVIDE,              /* 133. */
  CLOSEPAREN,      MODULUS,             /* 134. */
  CLOSEPAREN,      ASL,                 /* 135. */
  CLOSEPAREN,      ASR,                 /* 136. */
  CLOSEPAREN,      LT,                  /* 137. */
  CLOSEPAREN,      LE,                  /* 138. */
  CLOSEPAREN,      GT,                  /* 139. */
  CLOSEPAREN,      GE,                  /* 140. */
  CLOSEPAREN,      BOOLEAN_EQ,          /* 141. */
  CLOSEPAREN,      INEQUALITY,          /* 142. */
  CLOSEPAREN,      AMPERSAND,           /* 143. */
  CLOSEPAREN,      BIT_OR,              /* 144. */
  CLOSEPAREN,      BIT_XOR,             /* 145. */
  CLOSEPAREN,      BOOLEAN_AND,         /* 146. */
  CLOSEPAREN,      BOOLEAN_OR,          /* 147. */
  CLOSEPAREN,      EQ,                  /* 148. */
  CLOSEPAREN,      ASR_ASSIGN,          /* 149. */
  CLOSEPAREN,      ASL_ASSIGN,          /* 150. */
  CLOSEPAREN,      PLUS_ASSIGN,         /* 151. */
  CLOSEPAREN,      MINUS_ASSIGN,        /* 152. */
  CLOSEPAREN,      MULT_ASSIGN,         /* 153. */
  CLOSEPAREN,      DIV_ASSIGN,          /* 154. */
  CLOSEPAREN,      BIT_AND_ASSIGN,      /* 155. */
  CLOSEPAREN,      BIT_OR_ASSIGN,       /* 156. */
  CLOSEPAREN,      BIT_XOR_ASSIGN,      /* 157. */

  DEREF, LABEL,              /* 158. */
  PLUS, LABEL,               /* 159. */
  MINUS, LABEL,              /* 160. */
  ASTERISK, LABEL,           /* 161. */
  DIVIDE, LABEL,             /* 162. */
  MODULUS, LABEL,            /* 163. */
  ASL, LABEL,                /* 164. */
  ASR, LABEL,                /* 165. */
  LT, LABEL,                 /* 166. */
  LE, LABEL,                 /* 167. */
  GT, LABEL,                 /* 168. */
  GE, LABEL,                 /* 169. */
  BOOLEAN_EQ, LABEL,         /* 170. */
  INEQUALITY, LABEL,         /* 171. */
  AMPERSAND, LABEL,          /* 172. */
  BIT_OR, LABEL,             /* 173. */
  BIT_XOR, LABEL,            /* 174. */
  BOOLEAN_AND, LABEL,        /* 175. */
  BOOLEAN_OR, LABEL,         /* 176. */
  EQ, LABEL,                 /* 177. */
  ASR_ASSIGN, LABEL,         /* 178. */
  ASL_ASSIGN, LABEL,         /* 179. */
  PLUS_ASSIGN, LABEL,        /* 180. */
  MINUS_ASSIGN, LABEL,       /* 181. */
  MULT_ASSIGN, LABEL,        /* 182. */
  DIV_ASSIGN, LABEL,         /* 183. */
  BIT_AND_ASSIGN, LABEL,     /* 184. */
  BIT_OR_ASSIGN, LABEL,      /* 185. */
  BIT_XOR_ASSIGN, LABEL,     /* 186. */
  CONDITIONAL, LABEL,        /* 187. */
  COLON, LABEL,              /* 188. */

  DEREF, OPENPAREN,              /* 189. */
  PLUS, OPENPAREN,               /* 190. */
  MINUS, OPENPAREN,              /* 191. */
  ASTERISK, OPENPAREN,           /* 192. */
  DIVIDE, OPENPAREN,             /* 193. */
  MODULUS, OPENPAREN,            /* 194. */
  ASL, OPENPAREN,                /* 195. */
  ASR, OPENPAREN,                /* 196. */
  LT, OPENPAREN,                 /* 197. */
  LE, OPENPAREN,                 /* 198. */
  GT, OPENPAREN,                 /* 199. */
  GE, OPENPAREN,                 /* 200. */
  BOOLEAN_EQ, OPENPAREN,         /* 201. */
  INEQUALITY, OPENPAREN,         /* 202. */
  AMPERSAND, OPENPAREN,          /* 203. */
  BIT_OR, OPENPAREN,             /* 204. */
  BIT_XOR, OPENPAREN,            /* 205. */
  BOOLEAN_AND, OPENPAREN,        /* 206. */
  BOOLEAN_OR, OPENPAREN,         /* 207. */
  EQ, OPENPAREN,                 /* 208. */
  ASR_ASSIGN, OPENPAREN,         /* 209. */
  ASL_ASSIGN, OPENPAREN,         /* 300. */
  PLUS_ASSIGN, OPENPAREN,        /* 301. */
  MINUS_ASSIGN, OPENPAREN,       /* 302. */
  MULT_ASSIGN, OPENPAREN,        /* 303. */
  DIV_ASSIGN, OPENPAREN,         /* 304. */
  BIT_AND_ASSIGN, OPENPAREN,     /* 305. */
  BIT_OR_ASSIGN, OPENPAREN,      /* 306. */
  BIT_XOR_ASSIGN, OPENPAREN,     /* 307. */
  CONDITIONAL, OPENPAREN,        /* 308. */
  COLON, OPENPAREN,              /* 309. */

  DEREF, CHAR,              /* 310. */
  PLUS, CHAR,               /* 311. */
  MINUS, CHAR,              /* 312. */
  ASTERISK, CHAR,           /* 313. */
  DIVIDE, CHAR,             /* 314. */
  MODULUS, CHAR,            /* 315. */
  ASL, CHAR,                /* 316. */
  ASR, CHAR,                /* 317. */
  LT, CHAR,                 /* 318. */
  LE, CHAR,                 /* 319. */
  GT, CHAR,                 /* 320. */
  GE, CHAR,                 /* 321. */
  BOOLEAN_EQ, CHAR,         /* 322. */
  INEQUALITY, CHAR,         /* 323. */
  AMPERSAND, CHAR,          /* 324. */
  BIT_OR, CHAR,             /* 325. */
  BIT_XOR, CHAR,            /* 326. */
  BOOLEAN_AND, CHAR,        /* 327. */
  BOOLEAN_OR, CHAR,         /* 328. */
  EQ, CHAR,                 /* 329. */
  ASR_ASSIGN, CHAR,         /* 330. */
  ASL_ASSIGN, CHAR,         /* 331. */
  PLUS_ASSIGN, CHAR,        /* 332. */
  MINUS_ASSIGN, CHAR,       /* 333. */
  MULT_ASSIGN, CHAR,        /* 334. */
  DIV_ASSIGN, CHAR,         /* 335. */
  BIT_AND_ASSIGN, CHAR,     /* 336. */
  BIT_OR_ASSIGN, CHAR,      /* 337. */
  BIT_XOR_ASSIGN, CHAR,     /* 338. */
  CONDITIONAL, CHAR,        /* 339. */
  COLON, CHAR,              /* 340. */

  DEREF, LITERAL,              /* 341. */
  PLUS, LITERAL,               /* 342. */
  MINUS, LITERAL,              /* 343. */
  ASTERISK, LITERAL,           /* 344. */
  DIVIDE, LITERAL,             /* 345. */
  MODULUS, LITERAL,            /* 346. */
  ASL, LITERAL,                /* 347. */
  ASR, LITERAL,                /* 348. */
  LT, LITERAL,                 /* 349. */
  LE, LITERAL,                 /* 350. */
  GT, LITERAL,                 /* 351. */
  GE, LITERAL,                 /* 352. */
  BOOLEAN_EQ, LITERAL,         /* 353. */
  INEQUALITY, LITERAL,         /* 354. */
  AMPERSAND, LITERAL,          /* 355. */
  BIT_OR, LITERAL,             /* 356. */
  BIT_XOR, LITERAL,            /* 357. */
  BOOLEAN_AND, LITERAL,        /* 358. */
  BOOLEAN_OR, LITERAL,         /* 359. */
  EQ, LITERAL,                 /* 360. */
  ASR_ASSIGN, LITERAL,         /* 361. */
  ASL_ASSIGN, LITERAL,         /* 362. */
  PLUS_ASSIGN, LITERAL,        /* 363. */
  MINUS_ASSIGN, LITERAL,       /* 364. */
  MULT_ASSIGN, LITERAL,        /* 365. */
  DIV_ASSIGN, LITERAL,         /* 366. */
  BIT_AND_ASSIGN, LITERAL,     /* 367. */
  BIT_OR_ASSIGN, LITERAL,      /* 368. */
  BIT_XOR_ASSIGN, LITERAL,     /* 369. */
  CONDITIONAL, LITERAL,        /* 370. */
  COLON, LITERAL,              /* 371. */

  DEREF, LITERAL_CHAR,              /* 372. */
  PLUS, LITERAL_CHAR,               /* 373. */
  MINUS, LITERAL_CHAR,              /* 374. */
  ASTERISK, LITERAL_CHAR,           /* 375. */
  DIVIDE, LITERAL_CHAR,             /* 376. */
  MODULUS, LITERAL_CHAR,            /* 377. */
  ASL, LITERAL_CHAR,                /* 378. */
  ASR, LITERAL_CHAR,                /* 379. */
  LT, LITERAL_CHAR,                 /* 380. */
  LE, LITERAL_CHAR,                 /* 381. */
  GT, LITERAL_CHAR,                 /* 382. */
  GE, LITERAL_CHAR,                 /* 383. */
  BOOLEAN_EQ, LITERAL_CHAR,         /* 384. */
  INEQUALITY, LITERAL_CHAR,         /* 385. */
  AMPERSAND, LITERAL_CHAR,          /* 386. */
  BIT_OR, LITERAL_CHAR,             /* 387. */
  BIT_XOR, LITERAL_CHAR,            /* 388. */
  BOOLEAN_AND, LITERAL_CHAR,        /* 389. */
  BOOLEAN_OR, LITERAL_CHAR,         /* 390. */
  EQ, LITERAL_CHAR,                 /* 391. */
  ASR_ASSIGN, LITERAL_CHAR,         /* 392. */
  ASL_ASSIGN, LITERAL_CHAR,         /* 393. */
  PLUS_ASSIGN, LITERAL_CHAR,        /* 394. */
  MINUS_ASSIGN, LITERAL_CHAR,       /* 395. */
  MULT_ASSIGN, LITERAL_CHAR,        /* 396. */
  DIV_ASSIGN, LITERAL_CHAR,         /* 397. */
  BIT_AND_ASSIGN, LITERAL_CHAR,     /* 398. */
  BIT_OR_ASSIGN, LITERAL_CHAR,      /* 399. */
  BIT_XOR_ASSIGN, LITERAL_CHAR,     /* 400. */
  CONDITIONAL, LITERAL_CHAR,        /* 401. */
  COLON, LITERAL_CHAR,              /* 402. */

  DEREF, INTEGER,              /* 403. */
  PLUS, INTEGER,               /* 404. */
  MINUS, INTEGER,              /* 405. */
  ASTERISK, INTEGER,           /* 406. */
  DIVIDE, INTEGER,             /* 407. */
  MODULUS, INTEGER,            /* 408. */
  ASL, INTEGER,                /* 409. */
  ASR, INTEGER,                /* 410. */
  LT, INTEGER,                 /* 411. */
  LE, INTEGER,                 /* 412. */
  GT, INTEGER,                 /* 413. */
  GE, INTEGER,                 /* 414. */
  BOOLEAN_EQ, INTEGER,         /* 415. */
  INEQUALITY, INTEGER,         /* 416. */
  AMPERSAND, INTEGER,          /* 417. */
  BIT_OR, INTEGER,             /* 418. */
  BIT_XOR, INTEGER,            /* 419. */
  BOOLEAN_AND, INTEGER,        /* 420. */
  BOOLEAN_OR, INTEGER,         /* 421. */
  EQ, INTEGER,                 /* 422. */
  ASR_ASSIGN, INTEGER,         /* 423. */
  ASL_ASSIGN, INTEGER,         /* 424. */
  PLUS_ASSIGN, INTEGER,        /* 425. */
  MINUS_ASSIGN, INTEGER,       /* 426. */
  MULT_ASSIGN, INTEGER,        /* 427. */
  DIV_ASSIGN, INTEGER,         /* 428. */
  BIT_AND_ASSIGN, INTEGER,     /* 429. */
  BIT_OR_ASSIGN, INTEGER,      /* 430. */
  BIT_XOR_ASSIGN, INTEGER,     /* 431. */
  CONDITIONAL, INTEGER,        /* 432. */
  COLON, INTEGER,              /* 433. */

  DEREF, DOUBLE,              /* 434. */
  PLUS, DOUBLE,               /* 435. */
  MINUS, DOUBLE,              /* 436. */
  ASTERISK, DOUBLE,           /* 437. */
  DIVIDE, DOUBLE,             /* 438. */
  MODULUS, DOUBLE,            /* 439. */
  ASL, DOUBLE,                /* 440. */
  ASR, DOUBLE,                /* 441. */
  LT, DOUBLE,                 /* 442. */
  LE, DOUBLE,                 /* 443. */
  GT, DOUBLE,                 /* 444. */
  GE, DOUBLE,                 /* 445. */
  BOOLEAN_EQ, DOUBLE,         /* 446. */
  INEQUALITY, DOUBLE,         /* 447. */
  AMPERSAND, DOUBLE,          /* 448. */
  BIT_OR, DOUBLE,             /* 449. */
  BIT_XOR, DOUBLE,            /* 450. */
  BOOLEAN_AND, DOUBLE,        /* 451. */
  BOOLEAN_OR, DOUBLE,         /* 452. */
  EQ, DOUBLE,                 /* 453. */
  ASR_ASSIGN, DOUBLE,         /* 454. */
  ASL_ASSIGN, DOUBLE,         /* 455. */
  PLUS_ASSIGN, DOUBLE,        /* 456. */
  MINUS_ASSIGN, DOUBLE,       /* 457. */
  MULT_ASSIGN, DOUBLE,        /* 458. */
  DIV_ASSIGN, DOUBLE,         /* 459. */
  BIT_AND_ASSIGN, DOUBLE,     /* 460. */
  BIT_OR_ASSIGN, DOUBLE,      /* 461. */
  BIT_XOR_ASSIGN, DOUBLE,     /* 462. */
  CONDITIONAL, DOUBLE,        /* 463. */
  COLON, DOUBLE,              /* 464. */

  DEREF, LONG,              /* 465. */
  PLUS, LONG,               /* 466. */
  MINUS, LONG,              /* 467. */
  ASTERISK, LONG,           /* 468. */
  DIVIDE, LONG,             /* 469. */
  MODULUS, LONG,            /* 470. */
  ASL, LONG,                /* 471. */
  ASR, LONG,                /* 472. */
  LT, LONG,                 /* 473. */
  LE, LONG,                 /* 474. */
  GT, LONG,                 /* 475. */
  GE, LONG,                 /* 476. */
  BOOLEAN_EQ, LONG,         /* 477. */
  INEQUALITY, LONG,         /* 478. */
  AMPERSAND, LONG,          /* 479. */
  BIT_OR, LONG,             /* 480. */
  BIT_XOR, LONG,            /* 481. */
  BOOLEAN_AND, LONG,        /* 482. */
  BOOLEAN_OR, LONG,         /* 483. */
  EQ, LONG,                 /* 484. */
  ASR_ASSIGN, LONG,         /* 485. */
  ASL_ASSIGN, LONG,         /* 486. */
  PLUS_ASSIGN, LONG,        /* 487. */
  MINUS_ASSIGN, LONG,       /* 488. */
  MULT_ASSIGN, LONG,        /* 489. */
  DIV_ASSIGN, LONG,         /* 490. */
  BIT_AND_ASSIGN, LONG,     /* 491. */
  BIT_OR_ASSIGN, LONG,      /* 492. */
  BIT_XOR_ASSIGN, LONG,     /* 493. */
  CONDITIONAL, LONG,        /* 494. */
  COLON, LONG,              /* 495. */

  DEREF, LONGLONG,              /* 496. */
  PLUS, LONGLONG,               /* 497. */
  MINUS, LONGLONG,              /* 498. */
  ASTERISK, LONGLONG,           /* 499. */
  DIVIDE, LONGLONG,             /* 500. */
  MODULUS, LONGLONG,            /* 501. */
  ASL, LONGLONG,                /* 502. */
  ASR, LONGLONG,                /* 503. */
  LT, LONGLONG,                 /* 504. */
  LE, LONGLONG,                 /* 505. */
  GT, LONGLONG,                 /* 506. */
  GE, LONGLONG,                 /* 507. */
  BOOLEAN_EQ, LONGLONG,         /* 508. */
  INEQUALITY, LONGLONG,         /* 509. */
  AMPERSAND, LONGLONG,          /* 510. */
  BIT_OR, LONGLONG,             /* 511. */
  BIT_XOR, LONGLONG,            /* 512. */
  BOOLEAN_AND, LONGLONG,        /* 513. */
  BOOLEAN_OR, LONGLONG,         /* 514. */
  EQ, LONGLONG,                 /* 515. */
  ASR_ASSIGN, LONGLONG,         /* 516. */
  ASL_ASSIGN, LONGLONG,         /* 517. */
  PLUS_ASSIGN, LONGLONG,        /* 518. */
  MINUS_ASSIGN, LONGLONG,       /* 519. */
  MULT_ASSIGN, LONGLONG,        /* 520. */
  DIV_ASSIGN, LONGLONG,         /* 521. */
  BIT_AND_ASSIGN, LONGLONG,     /* 522. */
  BIT_OR_ASSIGN, LONGLONG,      /* 523. */
  BIT_XOR_ASSIGN, LONGLONG,     /* 524. */
  CONDITIONAL, LONGLONG,        /* 525. */
  COLON, LONGLONG,              /* 526. */

  CHAR,      DEREF,               /* 527. */
  CHAR,      PLUS,                /* 528. */
  CHAR,      MINUS,               /* 529. */
  CHAR,      ASTERISK,            /* 530. */
  CHAR,      DIVIDE,              /* 531. */
  CHAR,      MODULUS,             /* 532. */
  CHAR,      ASL,                 /* 533. */
  CHAR,      ASR,                 /* 534. */
  CHAR,      LT,                  /* 535. */
  CHAR,      LE,                  /* 536. */
  CHAR,      GT,                  /* 537. */
  CHAR,      GE,                  /* 538. */
  CHAR,      BOOLEAN_EQ,          /* 539. */
  CHAR,      INEQUALITY,          /* 540. */
  CHAR,      AMPERSAND,           /* 541. */
  CHAR,      BIT_OR,              /* 542. */
  CHAR,      BIT_XOR,             /* 543. */
  CHAR,      BOOLEAN_AND,         /* 544. */
  CHAR,      BOOLEAN_OR,          /* 545. */
  CHAR,      EQ,                  /* 546. */
  CHAR,      ASR_ASSIGN,          /* 547. */
  CHAR,      ASL_ASSIGN,          /* 548. */
  CHAR,      PLUS_ASSIGN,         /* 549. */
  CHAR,      MINUS_ASSIGN,        /* 550. */
  CHAR,      MULT_ASSIGN,         /* 551. */
  CHAR,      DIV_ASSIGN,          /* 552. */
  CHAR,      BIT_AND_ASSIGN,      /* 553. */
  CHAR,      BIT_OR_ASSIGN,       /* 554. */
  CHAR,      BIT_XOR_ASSIGN,      /* 555. */
  CHAR,      CONDITIONAL,      /* 556. */
  CHAR,      COLON,      /* 557. */

  LABEL,      DEREF,               /* 558. */
  LABEL,      PLUS,                /* 559. */
  LABEL,      MINUS,               /* 560. */
  LABEL,      ASTERISK,            /* 561. */
  LABEL,      DIVIDE,              /* 562. */
  LABEL,      MODULUS,             /* 563. */
  LABEL,      ASL,                 /* 564. */
  LABEL,      ASR,                 /* 565. */
  LABEL,      LT,                  /* 566. */
  LABEL,      LE,                  /* 567. */
  LABEL,      GT,                  /* 568. */
  LABEL,      GE,                  /* 569. */
  LABEL,      BOOLEAN_EQ,          /* 570. */
  LABEL,      INEQUALITY,          /* 571. */
  LABEL,      AMPERSAND,           /* 572. */
  LABEL,      BIT_OR,              /* 573. */
  LABEL,      BIT_XOR,             /* 574. */
  LABEL,      BOOLEAN_AND,         /* 575. */
  LABEL,      BOOLEAN_OR,          /* 576. */
  LABEL,      EQ,                  /* 577. */
  LABEL,      ASR_ASSIGN,          /* 578. */
  LABEL,      ASL_ASSIGN,          /* 579. */
  LABEL,      PLUS_ASSIGN,         /* 580. */
  LABEL,      MINUS_ASSIGN,        /* 581. */
  LABEL,      MULT_ASSIGN,         /* 582. */
  LABEL,      DIV_ASSIGN,          /* 583. */
  LABEL,      BIT_AND_ASSIGN,      /* 584. */
  LABEL,      BIT_OR_ASSIGN,       /* 585. */
  LABEL,      BIT_XOR_ASSIGN,      /* 586. */
  LABEL,      CONDITIONAL,      /* 587. */
  LABEL,      COLON,      /* 588. */
    LABEL,      INTEGER,  /* 589 */
    LABEL,      DOUBLE,     /* 590 */
    LABEL,      LONG,         /* 591 */
    LABEL,      LONGLONG,      /* 592 */

  /* More unusual states. */
  EXCLAM, OPENPAREN,             /* 593. */
  EXCLAM, LABEL,             /* 594. */
  EXCLAM, CHAR,             /* 595. */
  EXCLAM, LITERAL,             /* 596. */
  EXCLAM, LITERAL_CHAR,             /* 597. */
  EXCLAM, INTEGER,             /* 598. */
  EXCLAM, DOUBLE,             /* 599. */
 EXCLAM, LONG,             /* 600. */
 EXCLAM, LONGLONG,             /* 601. */

  CLOSEPAREN, CONDITIONAL,       /* 602. */
  CLOSEPAREN, COLON,             /* 603. */
    ARRAYOPEN,  OPENPAREN, /* 604. */
    CLOSEPAREN, ARRAYCLOSE, /* 605. */
    /*
     *
     */
    LABEL, ARRAYOPEN,  /* 606 */
    ARRAYOPEN, ARRAYCLOSE, /* 607 */
    ARRAYOPEN, OPENPAREN, /* 608 */
  ARRAYOPEN,      DEREF,               /* 609. */
  ARRAYOPEN,      PLUS,                /* 610. */
  ARRAYOPEN,      MINUS,               /* 611. */
  ARRAYOPEN,      ASTERISK,            /* 612. */
  ARRAYOPEN,      DIVIDE,              /* 613. */
  ARRAYOPEN,      MODULUS,             /* 614. */
  ARRAYOPEN,      ASL,                 /* 615. */
  ARRAYOPEN,      ASR,                 /* 616. */
  ARRAYOPEN,      LT,                  /* 617. */
  ARRAYOPEN,      LE,                  /* 618. */
  ARRAYOPEN,      GT,                  /* 619. */
  ARRAYOPEN,      GE,                  /* 620. */
  ARRAYOPEN,      BOOLEAN_EQ,          /* 621. */
  ARRAYOPEN,      INEQUALITY,          /* 622. */
  ARRAYOPEN,      AMPERSAND,           /* 623. */
  ARRAYOPEN,      BIT_OR,              /* 624. */
  ARRAYOPEN,      BIT_XOR,             /* 625. */
  ARRAYOPEN,      BOOLEAN_AND,         /* 626. */
  ARRAYOPEN,      BOOLEAN_OR,          /* 627. */
  ARRAYOPEN,      EQ,                  /* 628. */
  ARRAYOPEN,      ASR_ASSIGN,          /* 629. */
  ARRAYOPEN,      ASL_ASSIGN,          /* 630. */
  ARRAYOPEN,      PLUS_ASSIGN,         /* 631. */
  ARRAYOPEN,      MINUS_ASSIGN,        /* 632. */
  ARRAYOPEN,      MULT_ASSIGN,         /* 633. */
  ARRAYOPEN,      DIV_ASSIGN,          /* 634. */
  ARRAYOPEN,      BIT_AND_ASSIGN,      /* 635. */
  ARRAYOPEN,      BIT_OR_ASSIGN,       /* 636. */
  ARRAYOPEN,      BIT_XOR_ASSIGN,      /* 637. */
  ARRAYOPEN,      CONDITIONAL,      /* 638. */
  ARRAYOPEN,      COLON,      /* 639. */

    CHAR, ARRAYCLOSE,  /* 640. */
    LABEL, ARRAYCLOSE, /* 641. */
    LITERAL, ARRAYCLOSE, /* 642. */
    LITERAL_CHAR, ARRAYCLOSE, /* 643. */
    INTEGER, ARRAYCLOSE, /* 644. */ 
    DOUBLE, ARRAYCLOSE, /* 645. */
    LONG, ARRAYCLOSE, /* 646. */
    LONGLONG, ARRAYCLOSE, /* 647. */
    CLOSEPAREN, ARRAYCLOSE, /* 648. */

  ASTERISK, ARRAYOPEN,


