import sqlite3
from flask import Flask, request, jsonify, render_template_string

DATABASE = 'app.db'

def get_db_connection():
    conn = sqlite3.connect(DATABASE)
    conn.row_factory = sqlite3.Row  # Esto permite acceder a las columnas por nombre.
    return conn

def init_db():
    """Crea la tabla sensor_data si no existe."""
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute('''
        CREATE TABLE IF NOT EXISTS sensor_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            peso INTEGER,
            pequeno INTEGER,
            mediano INTEGER,
            grande INTEGER,
            jumbo INTEGER
        )
    ''')
    conn.commit()
    conn.close()

app = Flask(__name__)

# Variable global para almacenar el último dato recibido (útil para la actualización en tiempo real)
datos_actuales = {
    "peso": 0,
    "pequeno": 0,
    "mediano": 0,
    "grande": 0,
    "jumbo": 0
}

@app.route("/update", methods=["POST"])
def update():
    """
    Recibe datos del sensor en formato JSON, actualiza la variable global
    y guarda el registro en la base de datos.
    Se esperan los campos: peso, pequeno, mediano, grande, jumbo.
    """
    if request.is_json:
        data = request.get_json()
        datos_actuales["peso"] = data.get("peso", 0)
        datos_actuales["pequeno"] = data.get("pequeno", 0)
        datos_actuales["mediano"] = data.get("mediano", 0)
        datos_actuales["grande"] = data.get("grande", 0)
        datos_actuales["jumbo"] = data.get("jumbo", 0)
        
        # Guardar en la base de datos
        conn = get_db_connection()
        cur = conn.cursor()
        cur.execute('''
            INSERT INTO sensor_data (peso, pequeno, mediano, grande, jumbo)
            VALUES (?, ?, ?, ?, ?)
        ''', (datos_actuales["peso"], datos_actuales["pequeno"], datos_actuales["mediano"],
              datos_actuales["grande"], datos_actuales["jumbo"]))
        conn.commit()
        conn.close()
        
        print("Datos actualizados:", datos_actuales)
        return "Datos recibidos", 200
    else:
        return "La petición no contiene JSON", 400

@app.route("/data")
def data():
    """Retorna los datos actuales en formato JSON (útil para la actualización en tiempo real)."""
    return jsonify(datos_actuales)

@app.route("/")
def index():
    """Página principal que muestra los datos en tiempo real."""
    html_content = '''
    <!DOCTYPE html>
    <html lang="es">
    <head>
      <meta charset="UTF-8">
      <title>Datos del Sensor</title>
      <style>
        body { background-color: #f5f5f5; font-family: Arial, sans-serif; color: #212121; margin: 0; padding: 0; }
        .container { max-width: 600px; margin: 40px auto; padding: 20px; background-color: #ffffff;
                     border: 1px solid #ddd; border-radius: 4px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { color: #007F00; text-align: center; margin-bottom: 20px; font-size: 1.8em; }
        .data-item { padding: 10px; margin-bottom: 10px; border-bottom: 1px solid #eee; }
        .data-item:last-child { border-bottom: none; }
        .label { font-weight: bold; margin-right: 10px; }
        .nav { text-align: center; margin-bottom: 20px; }
        .nav a { color: #007F00; text-decoration: none; margin: 0 10px; }
      </style>
    </head>
    <body>
      <div class="container">
        <div class="nav">
          <a href="/">Sensor Data</a> | <a href="/list">Ver Registro</a>
        </div>
        <h1>Datos del Sensor</h1>
        <div class="data-item"><span class="label">Peso:</span> <span id="peso">0</span> g</div>
        <div class="data-item"><span class="label">Huevo pequeño:</span> <span id="pequeno">0</span></div>
        <div class="data-item"><span class="label">Huevo mediano:</span> <span id="mediano">0</span></div>
        <div class="data-item"><span class="label">Huevo grande:</span> <span id="grande">0</span></div>
        <div class="data-item"><span class="label">Huevo jumbo:</span> <span id="jumbo">0</span></div>
      </div>
      <script>
          // Función que consulta la ruta /data cada segundo y actualiza los valores
          function actualizarDatos() {
              fetch('/data')
                  .then(response => response.json())
                  .then(data => {
                      document.getElementById('peso').innerText = data.peso;
                      document.getElementById('pequeno').innerText = data.pequeno;
                      document.getElementById('mediano').innerText = data.mediano;
                      document.getElementById('grande').innerText = data.grande;
                      document.getElementById('jumbo').innerText = data.jumbo;
                  })
                  .catch(error => console.error('Error al obtener datos:', error));
          }
          setInterval(actualizarDatos, 1000);
      </script>
    </body>
    </html>
    '''
    return render_template_string(html_content)

@app.route("/list")
def list_data():
    """Página que muestra un listado de todos los registros almacenados en la base de datos."""
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute('SELECT * FROM sensor_data ORDER BY timestamp DESC')
    rows = cur.fetchall()
    conn.close()
    
    html_content = '''
    <!DOCTYPE html>
    <html lang="es">
    <head>
      <meta charset="UTF-8">
      <title>Listado de Datos</title>
      <style>
        body { background-color: #f5f5f5; font-family: Arial, sans-serif; color: #212121; margin: 0; padding: 0; }
        .container { max-width: 800px; margin: 40px auto; padding: 20px; background-color: #ffffff;
                     border: 1px solid #ddd; border-radius: 4px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { color: #007F00; text-align: center; margin-bottom: 20px; font-size: 1.8em; }
        table { width: 100%; border-collapse: collapse; }
        th, td { padding: 10px; border-bottom: 1px solid #ddd; text-align: center; }
        th { background-color: #f0f0f0; }
        .nav { text-align: center; margin-bottom: 20px; }
        .nav a { color: #007F00; text-decoration: none; margin: 0 10px; }
      </style>
    </head>
    <body>
      <div class="container">
        <div class="nav">
          <a href="/">Sensor Data</a> | <a href="/list">Ver Registro</a>
        </div>
        <h1>Listado de Datos del Sensor</h1>
        <table>
          <tr>
            <th>ID</th>
            <th>Timestamp</th>
            <th>Peso</th>
            <th>Huevo Pequeño</th>
            2<th>Huevo Mediano</th>
            <th>Huevo Grande</th>
            <th>Huevo Jumbo</th>
          </tr>
          {% for row in rows %}
          <tr>
            <td>{{ row.id }}</td>
            <td>{{ row.timestamp }}</td>
            <td>{{ row.peso }}</td>
            <td>{{ row.pequeno }}</td>
            <td>{{ row.mediano }}</td>
            <td>{{ row.grande }}</td>
            <td>{{ row.jumbo }}</td>
          </tr>
          {% endfor %}
        </table>
      </div>
    </body>
    </html>
    '''
    return render_template_string(html_content, rows=rows)

if __name__ == '__main__':
    init_db()
    app.run(host="0.0.0.0", port=5000, debug=True)

