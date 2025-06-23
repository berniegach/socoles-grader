--
-- PostgreSQL database dump
--

-- Dumped from database version 13.11 (Debian 13.11-0+deb11u1)
-- Dumped by pg_dump version 13.11 (Debian 13.11-0+deb11u1)


--
-- Name: dept; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE dept (
    deptno numeric(2,0) DEFAULT NULL::numeric,
    dname character varying(14) DEFAULT NULL::character varying,
    loc character varying(13) DEFAULT NULL::character varying
);


--
-- Name: emp; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE emp (
    empno numeric(4,0) NOT NULL,
    ename character varying(10) DEFAULT NULL::character varying,
    job character varying(9) DEFAULT NULL::character varying,
    mgr numeric(4,0) DEFAULT NULL::numeric,
    hiredate date,
    sal numeric(7,2) DEFAULT NULL::numeric,
    comm numeric(7,2) DEFAULT NULL::numeric,
    deptno numeric(2,0) DEFAULT NULL::numeric
);


--
-- Data for Name: dept; Type: TABLE DATA; Schema: public; Owner: -
--

INSERT INTO dept VALUES (10, 'ACCOUNTING', 'NEW YORK');
INSERT INTO dept VALUES (20, 'RESEARCH', 'DALLAS');
INSERT INTO dept VALUES (30, 'SALES', 'CHICAGO');
INSERT INTO dept VALUES (40, 'OPERATIONS', 'BOSTON');


--
-- Data for Name: emp; Type: TABLE DATA; Schema: public; Owner: -
--

INSERT INTO emp VALUES (7369, 'SMITH', 'CLERK', 7902, '1980-12-17', 800.00, NULL, 20);
INSERT INTO emp VALUES (7499, 'ALLEN', 'SALESMAN', 7698, '1981-02-20', 1600.00, 300.00, 30);
INSERT INTO emp VALUES (7521, 'WARD', 'SALESMAN', 7698, '1981-02-22', 1250.00, 500.00, 30);
INSERT INTO emp VALUES (7566, 'JONES', 'MANAGER', 7839, '1981-04-02', 2975.00, NULL, 20);
INSERT INTO emp VALUES (7654, 'MARTIN', 'SALESMAN', 7698, '1981-09-28', 1250.00, 1400.00, 30);
INSERT INTO emp VALUES (7698, 'BLAKE', 'MANAGER', 7839, '1981-05-01', 2850.00, NULL, 30);
INSERT INTO emp VALUES (7782, 'CLARK', 'MANAGER', 7839, '1981-06-09', 2450.00, NULL, 10);
INSERT INTO emp VALUES (7788, 'SCOTT', 'ANALYST', 7566, '1982-12-09', 3000.00, NULL, 20);
INSERT INTO emp VALUES (7839, 'KING', 'PRESIDENT', NULL, '1981-11-17', 5000.00, NULL, 10);
INSERT INTO emp VALUES (7844, 'TURNER', 'SALESMAN', 7698, '1981-09-08', 1500.00, 0.00, 30);
INSERT INTO emp VALUES (7876, 'ADAMS', 'CLERK', 7788, '1983-01-12', 1100.00, NULL, 20);
INSERT INTO emp VALUES (7900, 'JAMES', 'CLERK', 7698, '1981-12-03', 950.00, NULL, 30);
INSERT INTO emp VALUES (7902, 'FORD', 'ANALYST', 7566, '1981-12-03', 3000.00, NULL, 20);
INSERT INTO emp VALUES (7934, 'MILLER', 'CLERK', 7782, '1982-01-23', 1300.00, NULL, 10);


--
-- PostgreSQL database dump complete
--

