# MIT License
#
# Copyright (c) 2025 Taylan Branco Meurer
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# SPDX-License-Identifier: MIT

from gi.repository import Adw
from gi.repository import Gtk
from .conectar import Connect
import math


@Gtk.Template(resource_path='/com/github/thorhent/CA/window.ui')
class ClinicalayudanteWindow(Adw.ApplicationWindow):
    __gtype_name__ = 'ClinicalayudanteWindow'

    entrySintomas = Gtk.Template.Child("entrySintomas")
    butAddSintomas = Gtk.Template.Child("butAddSintomas")
    adwExpandSintomas = Gtk.Template.Child("adwExpandSintomas")
    butRemSintomas = Gtk.Template.Child("butRemSintomas")
    #butInvestigar = Gtk.Template.Child("butInvestigar")
    enfermedadesListBox = Gtk.Template.Child("enfermedadesListBox")
    labelPosiblesEnfermedades = Gtk.Template.Child("labelPosiblesEnfermedades")
    tov = Gtk.Template.Child("tov")

    labelList = list() #lista de síntomas/signos en adwExpand

    iconsLista = ["network-cellular-signal-excellent-rtl-symbolic","network-cellular-signal-good-rtl-symbolic",
	"network-cellular-signal-ok-rtl-symbolic","network-cellular-signal-weak-rtl-symbolic"]
    ###########################################################

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.getListaBDSignosSintomas()
        
    
    def getListaBDSignosSintomas(self):
        try: 
            conn = Connect()
            cursor = conn.conectar()
            select = "select distinct sintomas_signos from síntomas_signos \
                order by sintomas_signos ASC;"

            sintomas = cursor.execute(select).fetchall()
            
            
            completion = Gtk.EntryCompletion()
            self.entrySintomas.set_completion(completion)

            list_store = Gtk.ListStore.new([str])
            for sintoma in sintomas:
                list_store.append(sintoma)

            completion.set_model(list_store)

            completion.set_text_column(0)
            completion.set_inline_completion(True)
            completion.set_inline_selection(True)

        except Exception as e:
            print(e)
            print("Falla conexión con base de datos.")
            toast = Adw.Toast(title="Falla en la conexión inicial con base de datos.")
            self.tov.add_toast(toast)


    @Gtk.Template.Callback("entrySintomas_enter")
    def enter_add_sintomas(self, *args):
        buffer = self.entrySintomas.get_text()
        label = Gtk.Label(label=buffer)
        self.labelList.append(label)
        self.adwExpandSintomas.add_row(label)
        self.entrySintomas.set_text("")
        self.entrySintomas.grab_focus_without_selecting()

    @Gtk.Template.Callback("butAddSintomas_clicked")
    def add_sintomas(self, *args):
        buffer = self.entrySintomas.get_text()
        label = Gtk.Label(label=buffer)
        self.labelList.append(label)
        self.adwExpandSintomas.add_row(label)
        self.entrySintomas.set_text("")
        self.entrySintomas.grab_focus_without_selecting()


    @Gtk.Template.Callback("butRemSintomas_clicked")
    def quit_sintomas(self, *args):
        if len(self.labelList) > 0:
            self.adwExpandSintomas.remove(self.labelList[len(self.labelList)-1].get_parent())
            self.labelList.pop()

    @Gtk.Template.Callback("butLimpiar_clicked")
    def limpiar_sintomas(self, *args):
        if len(self.labelList) > 0:
            for hijo in self.labelList:
                self.adwExpandSintomas.remove(hijo.get_parent())

            self.labelList.clear()


    
    @Gtk.Template.Callback("butInvestigar_clicked")
    def investigar_enfermedades(self, *args):
        try:
            obj_conn = Connect()
            cursor = obj_conn.conectar()
            select_base = """select síntomas_signos.*, enfermedades.enfermedad, enfermedades.síndrome, 
            etiologias.agente_etiologico from síntomas_signos inner join enfermedades  
            using(cod_enfermedad) inner join etiologias using(cod_etiologia) where """
            
            #un ciclo para crear una string select segun los signos/sintomas
            for i in range(len(self.labelList)):
                if i > 0:
                    select_sintomas += f" or sintomas_signos = '{self.labelList[i].get_text()}'" 
                else:
                    select_sintomas = f"sintomas_signos = '{self.labelList[i].get_text()}'"
            
            select_sintomas += ";"
            select_base += select_sintomas
            datos = cursor.execute(select_base).fetchall()
            
            
            print("######### SELECT DE LA BASE DE DATOS ########")
            print()
            print(datos)
            print()
            print(f"Tamanho = {len(datos)}")
            print()
            listaNova = self.ordenar_enfermedades(datos)
            self.escribir_enfermedades(listaNova)
            self.labelPosiblesEnfermedades.set_label(f"Posibles enfermedades [{len(listaNova)}]")
        except Exception:
            #cursor.close()
            #obj_conn.quit()
            toast = Adw.Toast(title="Agregue síntomas o signos.")
            self.tov.add_toast(toast)
            print("La lista de síntomas o signos no debe ser vacía.")
        
            
        
    
    def ordenar_enfermedades(self, listaDatos):
        #utilizar map con lambda para descobrir la cantidad de veces que la enfermedad se repite
        # utilizar zip para mezclar los datos
        listaEnfermedades = list()
        listaEAQ = list()
        tam_datos = len(listaDatos)
        
        ### agrega desde select en otra lista solo [Enfermedad , Síndrome , Agente etiologico] 
        for i in range(tam_datos):
            listaEnfermedades.append([listaDatos[i][4], listaDatos[i][5], listaDatos[i][6]])
        
        tam_enf = len(listaEnfermedades)
        
        #### crea otra lista con Enfermedad, sindrome, agente y cantidad de veces que enfermedad repite
        for i in range(tam_enf):
            listaEAQ.append(listaEnfermedades[i])
            listaEAQ[i].append(listaEnfermedades.count(listaEnfermedades[i]))
        
            
         ########## ordenar lista
        listaEAQ = sorted(listaEAQ, key=lambda x: x[3], reverse=True)
        
        ####generar nueva lista sin repeticiones de enfermedades preservando agentes
        listaNova = list()  
        listaEnf = list()
        #listaAg = list()
        for i in range(len(listaEAQ)):
            if listaEAQ[i][0] in listaEnf:
                for lista in listaNova:
                    if listaEAQ[i][0] == lista[0]:
                        if listaEAQ[i][2] not in lista:
                            lista.append(listaEAQ[i][2])
                            #listaAg.append(listaEAQ[i][2])
                
            else:
                listaNova.append(listaEAQ[i])
                listaEnf.append(listaEAQ[i][0])
                #listaAg.append(listaEAQ[i][2])
        
        print("##### LISTA NUEVA ORDENADA #######")
        print()
        print(listaNova)
        print()
        print(f"Tamaño = {len(listaNova)}")
        print()
        print("########## FIM ###########")
        print()
        return listaNova
    
        
    def escribir_enfermedades(self, listaNova):
        self.enfermedadesListBox.remove_all()
        
        tam_signos = len(self.labelList)
        
        for datos in listaNova:
            if datos[3]/tam_signos == 1:
                agentes = self.verificar_agentes(datos)
                if isinstance(agentes, list):
                    self.crear_adwExpandRow(datos[0], datos[1], agentes, self.iconsLista[0])
                else:
                    self.crear_adwActions(datos[0], datos[1], agentes, self.iconsLista[0])  
            elif datos[3]/tam_signos >= 0.6:
                agentes = self.verificar_agentes(datos)
                if isinstance(agentes, list):
                    self.crear_adwExpandRow(datos[0], datos[1], agentes, self.iconsLista[1])
                else:
                    self.crear_adwActions(datos[0], datos[1], agentes, self.iconsLista[1])
            elif datos[3]/tam_signos > 0.4:
                agentes = self.verificar_agentes(datos)
                if isinstance(agentes, list):
                    self.crear_adwExpandRow(datos[0], datos[1], agentes, self.iconsLista[2])
                else:
                    self.crear_adwActions(datos[0], datos[1], agentes, self.iconsLista[2])
            else:
                agentes = self.verificar_agentes(datos)
                if isinstance(agentes, list):
                    self.crear_adwExpandRow(datos[0], datos[1], agentes, self.iconsLista[3])
                else:
                    self.crear_adwActions(datos[0], datos[1], agentes, self.iconsLista[3])
    
    def verificar_agentes(self, datos):
        listaAgente = list()
        if len(datos) > 4:
            aux = len(datos)
            listaAgente.append(datos[2])
            while aux > 4: 
                listaAgente.append(datos[aux-1])
                aux -= 1
            return listaAgente  
        else:
            return datos[2]
    
    def crear_adwExpandRow(self, enfermedad, sindrome, agentes, icon):
        adwExpanderRow = Adw.ExpanderRow()
        adwExpanderRow.set_title(enfermedad)
        if sindrome != None:
            adwExpanderRow.set_subtitle(sindrome + f"   <b>Etiologias:</b> ")
        else:
            adwExpanderRow.set_subtitle(f"<b>Etiologias:</b> ")
            
        adwExpanderRow.set_icon_name(icon)
        
        adwExpanderRow.set_margin_top(5)
        adwExpanderRow.set_margin_start(10)
        adwExpanderRow.set_margin_end(10)
        adwExpanderRow.set_margin_bottom(5)
        
        for agente in agentes:
            #etiologia = Adw.ActionRow()
            #etiologia.set_title(f"<i>{agente}</i>")
            #etiologia.set_margin_start(25)
            #etiologia.set_margin_end(25)
            label = Gtk.Label(label=f"<i>{agente}</i>")
            label.set_use_markup(True)
            adwExpanderRow.add_row(label)
        
        botonEnfermedad = Gtk.Button()
        botonEnfermedad.set_name(f"{enfermedad}")
        botonEnfermedad.set_icon_name("edit-copy-symbolic")
        ###
        botonEnfermedad.set_margin_top(10)
        botonEnfermedad.set_margin_start(20)
        botonEnfermedad.set_margin_bottom(10)
        ####
        adwExpanderRow.add_suffix(botonEnfermedad)
        self.enfermedadesListBox.append(adwExpanderRow)
        
        self.conectar_boton(botonEnfermedad)
        
    def crear_adwActions(self, enfermedad, sindrome, agente, icon):
        adwAction = Adw.ActionRow()
        adwAction.activate()
        adwAction.set_title(enfermedad)
        if sindrome != None:
            subtitle = sindrome + " | <b>Etiología:</b> " + agente
            adwAction.set_subtitle(subtitle)
        else:
            adwAction.set_subtitle("<b>Etiología:</b> " + agente)
        
        adwAction.set_icon_name(icon)
        adwAction.set_margin_top(5)
        adwAction.set_margin_start(10)
        adwAction.set_margin_end(10)
        adwAction.set_margin_bottom(5)
        
        ### crear boton
        botonEnfermedad = Gtk.Button()
        ### identidad del boton
        botonEnfermedad.set_name(f"{enfermedad}")
        botonEnfermedad.set_icon_name("edit-copy-symbolic")
        ### ubicacion
        botonEnfermedad.set_margin_top(10)
        botonEnfermedad.set_margin_start(20)
        botonEnfermedad.set_margin_bottom(10)
        ####
        adwAction.add_suffix(botonEnfermedad)
        self.enfermedadesListBox.append(adwAction)
        
        self.conectar_boton(botonEnfermedad)
    
    
    def conectar_boton(self, botonEnfermedad):
        botonEnfermedad.connect("clicked", self.on_clicked)
    
    def on_clicked(self, boton):

        builder = Gtk.Builder()

        builder.add_from_resource('/com/github/thorhent/CA/enfermedad_window.ui')
        ## captura de objetos de la ventana
        window = builder.get_object("enfermedad_window")
        window.set_title(boton.get_name())
        ##### capturar objectos de la ventana
        ### page 4
        listBoxTratFarmacologico = builder.get_object("listBoxTratFarmacologico")
        ### page1
        gridSS = builder.get_object("gridSS")
        listBoxPreguntasP1 = builder.get_object("listBoxPreguntasP1")
        ### page2
        listBoxExploracion = builder.get_object("listBoxExploracion")
        ### page 3
        listBoxEstudios = builder.get_object("listBoxEstudios")
        #### statusPage
        statusPage1 = builder.get_object("statusPage1")
        statusPage2 = builder.get_object("statusPage2")
        statusPage3 = builder.get_object("statusPage3")
        statusPage4 = builder.get_object("statusPage4")
        
        try:
            #####
            conn = Connect()
            cursor = conn.conectar()
            select = f"select cod_enfermedad from enfermedades where enfermedad = '{boton.get_name()}';"
            cod_enfermedad = cursor.execute(select).fetchone()
            
            #### select para pagina de tratamiento
            select = f"select * from tratamientos where cod_enfermedad = {cod_enfermedad[0]};"
            datosTratamiento = cursor.execute(select).fetchall()
            
            ##### select para pagina de anamnesis
            select = f"select distinct sintomas_signos from síntomas_signos where cod_enfermedad = {cod_enfermedad[0]};"
            datosSS = cursor.execute(select).fetchall()
            
            ####### select preguntas
            select = f"select * from preguntas where cod_enfermedad = {cod_enfermedad[0]} ORDER BY cod_pregunta ASC;"
            datosPreguntas = cursor.execute(select).fetchall()
            
            #### select para exploracion fisica
            select = f"select * from exploraciones_fisicas where cod_enfermedad = {cod_enfermedad[0]};"
            datosExploracion = cursor.execute(select).fetchall()
            
            #### select para pagina de estudio
            select = f"select * from estudios where cod_enfermedad = {cod_enfermedad[0]};"
            datosEstudios = cursor.execute(select).fetchall()
            
        except:
            pass


        window.present()
        
        ### definir status de cada pagina segun enfermedad
        statusPage1.set_description(f"{boton.get_name()}")
        statusPage2.set_description(f"{boton.get_name()}")
        statusPage3.set_description(f"{boton.get_name()}")
        statusPage4.set_description(f"{boton.get_name()}")
        
        #### llamar funciones para cada pagina
        self.crear_tratamiento(datosTratamiento, listBoxTratFarmacologico)
        self.crear_anamnesis(datosSS, datosPreguntas, gridSS, listBoxPreguntasP1)
        self.crear_estudios(datosEstudios, listBoxEstudios)
        self.crear_exploracion_fisica(datosExploracion, listBoxExploracion)
    

    def crear_anamnesis(self, datos, datosPreguntas, gridSS, listBoxPreguntasP1):
        lineas = math.ceil(len(datos)/4)
        aux = 0
        for linea in range(lineas):
            for columna in range(4):
                if aux == len(datos):
                    break
                adwAction = Adw.ActionRow()
                adwAction.set_title(datos[aux][0])
                
                aux += 1
                
                adwAction.set_margin_top(5)
                adwAction.set_margin_start(10)
                adwAction.set_margin_end(10)
                adwAction.set_margin_bottom(5)
                adwAction.add_css_class("card")
                    
                gridSS.attach(adwAction, columna, linea, 1, 1)
        
        for dato in datosPreguntas:
            adwAction = Adw.ActionRow()
            adwAction.set_title(dato[2])
            
            #adwExpander.set_subtitle(dato[3])
            
            #labelPregunta = Gtk.Label(label=f"Objetivo: {dato[5]}")
            
            #adwExpander.add_row(labelObjetivo)
            
            adwAction.set_margin_top(5)
            adwAction.set_margin_start(10)
            adwAction.set_margin_end(10)
            adwAction.set_margin_bottom(5)
            
            listBoxPreguntasP1.append(adwAction)
    
            
    def crear_exploracion_fisica(self, datos, listBoxExploracion):
        try:
            if datos[0][2] != None:
                listaInpeccion = datos[0][2].split("; ")
                adwExpander = Adw.ExpanderRow()
                adwExpander.set_title("<b>Inspección</b>")
                
                adwExpander.set_margin_top(5)
                adwExpander.set_margin_start(10)
                adwExpander.set_margin_end(10)
                adwExpander.set_margin_bottom(5)
                
                for insp in listaInpeccion:
                    adwAcInsp = Adw.ActionRow()
                    adwAcInsp.set_margin_start(25)
                    adwAcInsp.set_margin_end(25)
                    adwAcInsp.set_title(insp)
                    adwExpander.add_row(adwAcInsp)
                
                listBoxExploracion.append(adwExpander)
                
            if datos[0][3] != None:
                listaPalpacion = datos[0][3].split("; ")
                adwExpander = Adw.ExpanderRow()
                adwExpander.set_title("<b>Palpación</b>")
                
                adwExpander.set_margin_top(5)
                adwExpander.set_margin_start(10)
                adwExpander.set_margin_end(10)
                adwExpander.set_margin_bottom(5)
                
                for palp in listaPalpacion:
                    adwAcPalp = Adw.ActionRow()
                    adwAcPalp.set_title(palp)
                    adwAcPalp.set_margin_start(25)
                    adwAcPalp.set_margin_end(25)
                    adwExpander.add_row(adwAcPalp)
                
                listBoxExploracion.append(adwExpander)
                
                
            if datos[0][4] != None:
                listaPercusion = datos[0][4].split("; ")
                adwExpander = Adw.ExpanderRow()
                adwExpander.set_title("<b>Percusión</b>")
                
                adwExpander.set_margin_top(5)
                adwExpander.set_margin_start(10)
                adwExpander.set_margin_end(10)
                adwExpander.set_margin_bottom(5)
                
                for perc in listaPercusion:
                    adwAcPerc = Adw.ActionRow()                    
                    adwAcPerc.set_title(perc)
                    adwAcPerc.set_margin_start(25)
                    adwAcPerc.set_margin_end(25)
                    adwExpander.add_row(adwAcPerc)
                
                listBoxExploracion.append(adwExpander)
                
                
            if datos[0][5] != None:
                listaAuscultacion = datos[0][5].split("; ")
                adwExpander = Adw.ExpanderRow()
                adwExpander.set_title("<b>Auscultación</b>")
                
                adwExpander.set_margin_top(5)
                adwExpander.set_margin_start(10)
                adwExpander.set_margin_end(10)
                adwExpander.set_margin_bottom(5)
                
                for aus in listaAuscultacion:
                    adwAcAus = Adw.ActionRow()
                    adwAcAus.set_title(aus)
                    adwAcAus.set_margin_start(25)
                    adwAcAus.set_margin_end(25)
                    adwExpander.add_row(adwAcAus)
                
                listBoxExploracion.append(adwExpander)
                
        except Exception as error:
            #print(f"Se espera {error=}, {type(error)=}")
            print("Ausencia de datos de exploración física.")
            
             
    def crear_estudios(self, datos, listBoxEstudios):
        try:
            for dato in datos:
                listaEstudioObjetivo = dato[4].split("; ")
                adwExpander = Adw.ExpanderRow()
                adwExpander.set_title(f"<b>{dato[3]}</b>")
                adwExpander.set_subtitle(f"Estudio: {dato[2]}")
                label = Gtk.Label(label="Objetivo(s)")
                adwExpander.add_suffix(label)
                for objetivo in listaEstudioObjetivo:
                    adwAcObjetivo = Adw.ActionRow()
                    adwAcObjetivo.set_title(objetivo)
                    adwAcObjetivo.set_margin_start(25)
                    adwAcObjetivo.set_margin_end(25)
                    adwExpander.add_row(adwAcObjetivo)
                    
                
                adwExpander.set_margin_top(5)
                adwExpander.set_margin_start(10)
                adwExpander.set_margin_end(10)
                adwExpander.set_margin_bottom(5)
                
                listBoxEstudios.append(adwExpander)
                
        except Exception as error:
            print("Ausencia de estudios")

    def crear_tratamiento(self, datos, listBoxTratFarmacologico):
        
        for dato in datos:
            #crear objeto Adw
            adwExpander = Adw.ExpanderRow()
            adwExpander.set_margin_top(5)
            adwExpander.set_margin_start(10)
            adwExpander.set_margin_end(10)
            adwExpander.set_margin_bottom(5)
            
            adwExpander.set_title(f"<b>{dato[4]}</b>")
            adwExpander.set_subtitle("<b>Clase:</b> "+ dato[3] + 
                f"   <b>Tipo:</b> {dato[2]}")
            
            adwAcObjetivo = Adw.ActionRow()
            adwAcObjetivo.set_title("Objetivo")
            adwAcObjetivo.set_subtitle(dato[5])
            adwAcObjetivo.set_margin_start(25)
            adwAcObjetivo.set_margin_end(25)
            
            adwExpander.add_row(adwAcObjetivo)
            
            if dato[6] != None:
                adwAcOtro = Adw.ActionRow()
                adwAcOtro.set_title("Otras informaciones")
                adwAcOtro.set_subtitle(dato[6])
                adwAcOtro.set_margin_start(25)
                adwAcOtro.set_margin_end(25)
                adwExpander.add_row(adwAcOtro)
                    
            listBoxTratFarmacologico.append(adwExpander)

