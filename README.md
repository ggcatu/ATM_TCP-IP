# ATM_TCP-IP

Gabriel Gimenez 12-11006
Guillermo Betancourt 11-10103

Archivos: 
	Makefile, contiene el makefile
	central.c, contiene el codigo fuente del servidor
	tcpechoclient.c, contiene el codigo fuente del cliente

Se creo un sistema de cajero automatico, que permite retirar y depositar dinero desde el cliente.
	
Para generar los ejecutables:
	make

El cajero automatico consta de 2 partes.

El servidor, puede ser invocado en base a las siguientes especificaciones
	bsb_svr -l <puerto_bsb_svr> -i <bitácora_depósito> - o <bitácora_retiro>

Dónde:
	<puerto_bsb_svr> Es el número de puerto local en que el computador central
	ofrecerá el servicio
	<bitácora_depósito> Es el nombre y dirección relativa o absoluta de un archivo de
	texto que almacena las operaciones de depósito del cajero.
	<bitácora_retiro> Es el nombre y dirección relativa o absoluta de un archivo de
	texto que almacena las operaciones de retiro del cajero.

El cliente, puede ser invocado en base a las siguientes especificaciones
	bsb_cli -d <nombre_módulo_atención> -p <puerto_bsb_svr> - c <op> -i <codigo_usuario>

Dónde:
	<nombre_módulo_atención>: Es el nombre de dominio o la dirección IP (versión 4)
	del equipo en donde se deberá ejecutar el módulo de atención centralizada.
	<puerto_bsb_svr>: Es el número de puerto remoto en que el módulo de servicio
	atenderá la comunicación solicitada.
	<op>: Indica si el usuario va a depositar o retirar dinero del cajero, puede tener
	dos valores d ó r
	<código_usuario> Es un número serial que identifica un usuario de forma única

Durante la creacion se utilizaron como codigo_usuario, numeros de 4 digitos.

Condiciones:
	Servidor:
		Maximo de clientes 100
		Monto inicial del cajero 8000
		Maximo monto de retiro 3000
		Numero de retiros diarios permitidos MAXNRETIROS 3
		Largo maximo de mensaje del protocolo 300
		Aviso al cliente cuando se posee menos de 5000
		Cada cliente tiene que poseer un identificador de maximo 100 caracteres
		Las fechas poseen maximo 30 caracteres
		Los errores poseen maximo 100 caracteres
		Los movimientos de las bitacoras poseen maximo 200 caracteres
		Se utiliza el caracter especial '`' para encriptar los mensajes
		Los montos recibidos desde el cliente deben tener maximo 10 caracteres

	Cliente:
		Largo maximo de mensaje del protocolo 300
		Numero maximo de intentos para conectar al servidor 3
		// Codigo usuario 5 bytes?
